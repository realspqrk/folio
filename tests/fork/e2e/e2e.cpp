// The real app driven offscreen, for end-to-end tests and screenshots for
// visual review without a display: links the app's own main(), then, once a
// window is up, runs each step of FOLIO_E2E ("name=js;name=js;…") or of the
// FOLIO_E2E_STEPS file (one per line) as JavaScript against the
// window's root (Main.qml's scope; `bar` is the BookmarksBar, `layer` the
// BookmarksLayer with its bubble, dialog and confirm) and saves
// <FOLIO_E2E_DIR>/<n>-<name>.png. Steps named resize (WxH), click, rclick,
// mclick, press, move, release (X,Y, or @js giving "X,Y"), key (a key
// sequence: Ctrl+B) and type (text) send real input instead; assert (js)
// fails the run unless it's true. A name ending in "_" saves no picture.
// Helpers: chip(name) and at(name, fraction of its width) give a chip's
// point, names(list) the bookmarks' names, here() the active pane's path. Exits 1 on a failed assert or
// any QML warning, so a steps file is also an end-to-end test.
//
//   QT_QPA_PLATFORM=offscreen FOLIO_E2E_DIR=out FOLIO_E2E='main=;key-hide=Ctrl+Shift+B' ./folio-e2e
// tests/fork/e2e/run.sh sets up a made-up home and runs a steps file.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QtTest/QTest>
#include <cstdio>

static QQuickItem *findType(QQuickItem *item, const char *prefix) {
    if (QString(item->metaObject()->className()).startsWith(prefix))
        return item;
    for (QQuickItem *child : item->childItems())
        if (QQuickItem *found = findType(child, prefix))
            return found;
    return nullptr;
}

// Where the pointer was last put, for moves to start from (offscreen, the
// cursor doesn't follow test events).
static QPoint s_pointer;
static int s_failures = 0;

// Helpers every step can use.
static const char *s_helpers =
    "function at(name, fx) { for (const c of bar.chipRow.children) if (c.label === name && c.visible) {"
    " const p = c.mapToItem(null, c.width * fx, c.height / 2); return Math.round(p.x) + ',' + Math.round(p.y) } return 'none' }\n"
    "function chip(name) { return at(name, 0.5) }\n"
    "function names(list) { return (list || bookmarks.items).map(i => i.name).join(',') }\n"
    "function here() { return win.activePane.paneBackend.path }\n";

static QVariant evaluate(QQuickWindow *window, const QString &name, const QString &js) {
    // `bar` is found by type and handed in through a context of its own.
    QQmlContext context(QQmlEngine::contextForObject(window));
    context.setContextProperty("bar", findType(window->contentItem(), "BookmarksBar"));
    context.setContextProperty("layer", findType(window->contentItem(), "BookmarksLayer"));
    QQmlExpression run(&context, window, "(function() {" + QString(s_helpers) + "return (" + js + ") })()");
    const QVariant result = run.evaluate();
    if (run.hasError()) {
        fprintf(stderr, "step %s: %s\n", qPrintable(name), qPrintable(run.error().toString()));
        ++s_failures;
    }
    return result;
}

static void runStep(QStringList steps, int n) {
    const QString dir = qEnvironmentVariable("FOLIO_E2E_DIR");
    QQuickWindow *window = nullptr;
    for (QWindow *w : QGuiApplication::topLevelWindows())
        if (auto *q = qobject_cast<QQuickWindow *>(w); q && q->isVisible() && !window)
            window = q;
    if (steps.isEmpty() || !window) {
        if (!window)
            ++s_failures;
        fprintf(stderr, "%s\n", s_failures ? "e2e: FAILED" : "e2e: passed");
        QCoreApplication::exit(s_failures ? 1 : 0);
        return;
    }
    const QString step = steps.takeFirst();
    const QString name = step.section('=', 0, 0);
    const QString js = step.section('=', 1);
    const QString kind = name.section('-', 0, 0);
    const QString where = js.startsWith('@') ? evaluate(window, name, js.mid(1)).toString() : js;
    const QPoint point(where.section(',', 0, 0).toInt(), where.section(',', 1).toInt());
    if (kind == "click" || kind == "rclick" || kind == "mclick" || kind == "press" || kind == "rpress" || kind == "release")
        s_pointer = point;
    if (kind == "resize") {
        window->resize(js.section('x', 0, 0).toInt(), js.section('x', 1).toInt());
    } else if (kind == "click" || kind == "rclick" || kind == "mclick") {
        const Qt::MouseButton button = kind == "click" ? Qt::LeftButton : kind == "rclick" ? Qt::RightButton : Qt::MiddleButton;
        QTest::mouseMove(window, point);
        QTest::mouseClick(window, button, Qt::NoModifier, point);
    } else if (kind == "rpress") {
        QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, point);
    } else if (kind == "press") {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    } else if (kind == "move") {
        // In steps, as a hand does, so drag thresholds and drop areas see it.
        const QPoint from = s_pointer;
        for (int i = 1; i <= 8; ++i)
            QTest::mouseMove(window, from + (point - from) * i / 8);
        s_pointer = point;
    } else if (kind == "release") {
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    } else if (kind == "key") {
        const QKeyCombination combo = QKeySequence(js)[0];
        QTest::keyClick(window, combo.key(), combo.keyboardModifiers());
    } else if (kind == "type") {
        for (const QChar c : js)
            QTest::keyClick(window, c.toLatin1());
    } else if (kind == "assert") {
        if (evaluate(window, name, js).toBool() != true) {
            fprintf(stderr, "FAIL %s: %s\n", qPrintable(name), qPrintable(js));
            ++s_failures;
        } else {
            fprintf(stderr, "PASS %s\n", qPrintable(name));
        }
    } else if (!js.trimmed().isEmpty()) {
        const QVariant result = evaluate(window, name, js);
        if (result.isValid() && !result.isNull())
            fprintf(stderr, "step %s => %s\n", qPrintable(name), qPrintable(result.toString()));
    }
    // Asserts check what's there already; everything else gets a moment
    // to settle (menus, re-reads) before the next step or the picture.
    QTimer::singleShot(kind == "assert" ? 0 : 400, [=] {
        if (name.endsWith('_') || kind == "assert") {
            runStep(steps, n + 1);
            return;
        }
        const QString file = QDir(dir).filePath(QString("%1-%2.png").arg(n, 2, 10, QChar('0')).arg(name));
        if (!window->grabWindow().save(file))
            ++s_failures;
        fprintf(stderr, "saved %s\n", qPrintable(file));
        runStep(steps, n + 1);
    });
}

static void start() {
    if (qEnvironmentVariableIsEmpty("FOLIO_E2E_DIR"))
        return;
    // A QML warning is a failure: a broken binding often shows only there.
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &message) {
        fprintf(stderr, "%s\n", qPrintable(message));
        // Upstream's tiles say "No thumbnail" for a file no thumbnailer
        // takes (and draw a stand-in): a state, not a broken binding.
        if (type != QtDebugMsg && type != QtInfoMsg && message.contains("qrc:/")
            && !message.contains("QQuickImage: No thumbnail"))
            ++s_failures;
    });
    // Or one step per line from FOLIO_E2E_STEPS; # starts a comment.
    QStringList steps = qEnvironmentVariable("FOLIO_E2E").split(';', Qt::SkipEmptyParts);
    QFile file(qEnvironmentVariable("FOLIO_E2E_STEPS"));
    if (file.open(QIODevice::ReadOnly))
        for (const QString &line : QString::fromUtf8(file.readAll()).split('\n'))
            if (!line.trimmed().isEmpty() && !line.trimmed().startsWith('#'))
                steps << line.trimmed();
    QTimer::singleShot(1500, [steps] { runStep(steps, 1); });
}
Q_COREAPP_STARTUP_FUNCTION(start)
