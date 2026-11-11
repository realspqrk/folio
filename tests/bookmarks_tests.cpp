// The bookmarks bar's model (src/bookmarks.*): the tree on disk, moves,
// paths as the path bar takes them, a file that isn't bookmarks never
// written over, and two windows' changes kept apart. In a file of its own,
// beside upstream's tests, so their changes never meet.

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>
#include <sys/stat.h>
#include <unistd.h>

#include "bookmarks.h"

static QStringList bookmarkNames(const QVariantList &items) {
    QStringList out;
    for (const QVariant &v : items)
        out << v.toMap().value("name").toString();
    return out;
}

static QByteArray readAll(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

static void writeAll(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(bytes);
}

// Files that aren't bookmarks, most of them good JSON: reading what can be
// read of them would lose the rest at the next save.
static const QList<QByteArray> notBookmarks{
    "{ \"bookmarks\": [ oops",
    "[]",
    R"({"version":1})",
    R"({"version":1,"bookmarks":{}})",
    R"({"version":2,"bookmarks":[]})",
    R"({"version":1,"bookmarks":[3]})",
    R"({"version":1,"bookmarks":[{"name":"F","children":{}}]})",
    R"({"version":1,"bookmarks":[{"name":"X"}]})",
    R"({"version":1,"bookmarks":[{"name":"X","path":7}]})",
    R"({"version":1,"bookmarks":[{"path":"/x"}]})",
    // There but empty (an edit cut short): not the same as no file.
    " \n\t",
};

class BookmarksTests : public QObject {
    Q_OBJECT

private slots:
    void keepATreeAcrossRestarts()
    {
        QTemporaryDir home;
        QDir(home.path()).mkpath("Work/clients/acme");
        QVERIFY(QFile(home.filePath("taxes.pdf")).open(QIODevice::WriteOnly));
        const QString file = home.filePath(".config/app/bookmarks.json");
        {
            Bookmarks b(file, home.path());
            QVERIFY(b.items().isEmpty());
            QVERIFY(!b.add({}, "Inbox", home.filePath("Work")).isEmpty());
            const QString clients = b.addFolder({}, "Clients");
            QVERIFY(!clients.isEmpty());
            // Named apart from the folder it goes to.
            QVERIFY(!b.add(clients, "ACME Corp", home.filePath("Work/clients/acme")).isEmpty());
            const QString archive = b.addFolder(clients, "Archive");
            QVERIFY(!b.add(archive, {}, home.filePath("taxes.pdf")).isEmpty());
            QVERIFY(b.problem().isEmpty());
        }
        Bookmarks b(file, home.path());
        const QVariantList items = b.items();
        QCOMPARE(bookmarkNames(items), (QStringList{"Inbox", "Clients"}));
        const QVariantMap inbox = items.at(0).toMap();
        QCOMPARE(inbox.value("path").toString(), QString("~/Work"));
        QCOMPARE(inbox.value("kind").toString(), QString("folder"));
        const QVariantList clients = items.at(1).toMap().value("children").toList();
        QCOMPARE(bookmarkNames(clients), (QStringList{"ACME Corp", "Archive"}));
        const QVariantMap taxes = clients.at(1).toMap().value("children").toList().at(0).toMap();
        QCOMPARE(taxes.value("name").toString(), QString("taxes.pdf"));
        QCOMPARE(taxes.value("kind").toString(), QString("file"));
        // The folder list for choosing where a bookmark goes, bar first.
        QCOMPARE(bookmarkNames(b.folders()), (QStringList{"Bookmarks Bar", "Clients", "Archive"}));
        QCOMPARE(b.folders().at(2).toMap().value("depth").toInt(), 2);

        // Renamed and pointed elsewhere; a folder keeps having no path.
        const QString id = inbox.value("id").toString();
        QVERIFY(b.edit(id, "Desk", "~/Work/clients").isEmpty());
        QCOMPARE(b.get(id).value("name").toString(), QString("Desk"));
        QCOMPARE(b.get(id).value("path").toString(), QString("~/Work/clients"));
        QCOMPARE(Bookmarks(file, home.path()).items().at(0).toMap().value("name").toString(), QString("Desk"));
    }

    void moveReorderAndRefuseCycles()
    {
        QTemporaryDir home;
        Bookmarks b(home.filePath("bookmarks.json"), home.path());
        const QString a = b.add({}, "A", "/a");
        const QString c = b.add({}, "C", "/c");
        const QString f = b.addFolder({}, "F");
        const QString sub = b.addFolder(f, "Sub");
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"A", "C", "F"}));

        // Index counts the bar as it is, as a drop between two chips does.
        QVERIFY(b.move(a, {}, 2));
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"C", "A", "F"}));
        QVERIFY(b.move(f, {}, 0));
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"F", "C", "A"}));

        // Into a folder, and into a folder inside it.
        QVERIFY(b.move(c, sub));
        QCOMPARE(b.get(c).value("parentId").toString(), sub);
        // A folder can't go inside itself or below itself.
        QVERIFY(!b.move(f, sub));
        QVERIFY(!b.move(f, f));
        QVERIFY(b.move(sub, {}));
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"F", "A", "Sub"}));

        // Removing a folder removes what's in it.
        QVERIFY(b.remove(sub));
        QVERIFY(b.get(c).isEmpty());
        QVERIFY(!b.remove(sub));
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"F", "A"}));
    }

    void pathsWorkLikeThePathBar()
    {
        QTemporaryDir home;
        QDir(home.path()).mkpath("Work");
        Bookmarks b(home.filePath("bookmarks.json"), home.path());
        // ~, file:// and relative paths are all one place, stored with ~.
        QCOMPARE(b.normalize("~/Work/"), QString("~/Work"));
        QCOMPARE(b.normalize(QUrl::fromLocalFile(home.filePath("Work")).toString()), QString("~/Work"));
        QCOMPARE(b.normalize("Work/../Work"), QString("~/Work"));
        QCOMPARE(b.normalize(home.path()), QString("~"));
        QCOMPARE(b.normalize("/etc//"), QString("/etc"));
        // Places that aren't paths stay as written.
        QCOMPARE(b.normalize("recent:"), QString("recent:"));
        QCOMPARE(b.resolve("~/Work"), home.filePath("Work"));
        QCOMPARE(b.defaultName("~"), QString("Home"));
        // Typed in a dialog: relative to the folder shown, else as written.
        QCOMPARE(b.absolute("sub/../x", "~/Work"), QString("~/Work/x"));
        QCOMPARE(b.absolute("x", "/etc"), QString("/etc/x"));
        QCOMPARE(b.absolute("~/a", "/etc"), QString("~/a"));
        QCOMPARE(b.absolute("/a", "~/Work"), QString("/a"));
        QCOMPARE(b.absolute("recent:", "~/Work"), QString("recent:"));
        // Recents isn't a folder: from home, as the path bar does.
        QCOMPARE(b.absolute("Docs", "recent:"), QString("~/Docs"));
        // The base is exact (a folder may end in a space), and a colon after
        // a slash is a file name's, not a scheme.
        QCOMPARE(b.absolute("Child", "~/Work/Target "), QString("~/Work/Target /Child"));
        QCOMPARE(b.absolute("nested/report:2026", "~/Work"), QString("~/Work/nested/report:2026"));

        const QString work = b.add({}, {}, home.filePath("Work"));
        QCOMPARE(b.get(work).value("name").toString(), QString("Work"));
        QCOMPARE(b.find("~/Work"), work);
        QCOMPARE(b.find(home.filePath("Work") + "/"), work);
        QVERIFY(b.find("~/Elsewhere").isEmpty());

        // Gone, then back: refresh() says so.
        const QString gone = b.add({}, "Later", "~/Later");
        QCOMPARE(b.get(gone).value("kind").toString(), QString("missing"));
        QDir(home.path()).mkdir("Later");
        QSignalSpy changed(&b, &Bookmarks::changed);
        b.refresh();
        QCOMPARE(changed.count(), 1);
        QCOMPARE(b.get(gone).value("kind").toString(), QString("folder"));
        b.refresh();
        QCOMPARE(changed.count(), 1); // nothing new, no rebuild

        // A drop adds each local item at the drop point, in order, in one save.
        QVERIFY(QFile(home.filePath("a.txt")).open(QIODevice::WriteOnly));
        changed.clear();
        QCOMPARE(b.addUrls({}, {QUrl::fromLocalFile(home.filePath("a.txt")), QUrl("https://example.com"),
                                QUrl::fromLocalFile(home.filePath("Later"))}, 1), 2);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(bookmarkNames(b.items()), (QStringList{"Work", "a.txt", "Later", "Later"}));
    }

    void neverOverwriteABrokenFile()
    {
        QTemporaryDir home;
        const QString file = home.filePath("bookmarks.json");
        for (const QByteArray &junk : notBookmarks) {
            writeAll(file, junk);
            Bookmarks b(file, home.path());
            QVERIFY2(!b.problem().isEmpty(), junk.constData());
            QVERIFY(b.items().isEmpty());
            QVERIFY(b.add({}, "X", "/x").isEmpty());
            QCOMPARE(readAll(file), junk);
        }

        // Fixed by hand: read again, and saving works.
        Bookmarks b(file, home.path());
        writeAll(file, R"({"version":1,"bookmarks":[{"name":"Fixed","path":"/tmp"}]})");
        QVERIFY(QTest::qWaitFor([&] { return b.problem().isEmpty(); }, 3000));
        QCOMPARE(bookmarkNames(b.items()), QStringList{"Fixed"});
        QVERIFY(!b.add({}, "X", "/x").isEmpty());

        // Spoiled again just before a change, before the watcher says so:
        // the change is refused rather than saved over it.
        QSignalSpy changed(&b, &Bookmarks::changed);
        for (const QByteArray &junk : notBookmarks) {
            writeAll(file, R"({"version":1,"bookmarks":[]})");
            QVERIFY(!b.add({}, "Y", "/y").isEmpty());
            writeAll(file, junk);
            changed.clear();
            QVERIFY(b.add({}, "Z", "/z").isEmpty());
            QCOMPARE(readAll(file), junk);
            QVERIFY(!b.problem().isEmpty());
            QCOMPARE(changed.count(), 1);
        }
    }

    void followAnotherProcess()
    {
        QTemporaryDir home;
        const QString file = home.filePath("cfg/bookmarks.json");
        Bookmarks first(file, home.path());
        Bookmarks second(file, home.path());
        QVERIFY(!second.add({}, "From the other window", "/tmp").isEmpty());
        QVERIFY(QTest::qWaitFor([&] { return first.items().size() == 1; }, 3000));
        QCOMPARE(bookmarkNames(first.items()), QStringList{"From the other window"});
    }

    void twoWindowsKeepBothChanges()
    {
        QTemporaryDir home;
        const QString file = home.filePath("cfg/bookmarks.json");
        Bookmarks first(file, home.path());
        Bookmarks second(file, home.path());
        // Neither has heard of the other's change when it makes its own.
        QVERIFY(!first.add({}, "X", "/x").isEmpty());
        QVERIFY(!second.add({}, "Y", "/y").isEmpty());
        QCOMPARE(bookmarkNames(Bookmarks(file, home.path()).items()), (QStringList{"X", "Y"}));
        QVERIFY(QTest::qWaitFor([&] { return first.items().size() == 2; }, 3000));
        QCOMPARE(bookmarkNames(first.items()), (QStringList{"X", "Y"}));
        QCOMPARE(bookmarkNames(second.items()), (QStringList{"X", "Y"}));

        // A partial edit (a name only) keeps what another window changed
        // in the other fields, even before this one has heard of it.
        const QString x = first.find("/x");
        const QString folder = second.addFolder({}, "F");
        QVERIFY(second.move(x, folder));
        QVERIFY(second.edit(x, "", "/elsewhere").isEmpty());
        QVERIFY(first.edit(x, "Renamed", "").isEmpty());
        const Bookmarks fresh(file, home.path());
        QCOMPARE(fresh.get(x).value("name").toString(), QString("Renamed"));
        QCOMPARE(fresh.get(x).value("parentId").toString(), folder);
        QCOMPARE(fresh.get(x).value("path").toString(), QString("/elsewhere"));
    }

    void idsLastAcrossChangesFromElsewhere()
    {
        QTemporaryDir home;
        const QString file = home.filePath("cfg/bookmarks.json");
        Bookmarks first(file, home.path());
        const QString folder = first.addFolder({}, "F");
        const QString a = first.add(folder, "A", "/a");
        {
            Bookmarks other(file, home.path());
            QVERIFY(!other.addFolder({}, "From elsewhere", 0).isEmpty());
        }
        QVERIFY(QTest::qWaitFor([&] { return first.items().size() == 2; }, 3000));
        // An editor still holding `a` edits the same bookmark.
        QCOMPARE(first.get(a).value("name").toString(), QString("A"));
        QCOMPARE(first.get(a).value("parentId").toString(), folder);
        QVERIFY(first.edit(a, "A2", {}).isEmpty());
        QCOMPARE(Bookmarks(file, home.path()).get(a).value("name").toString(), QString("A2"));

        // A file written by hand, without ids: every window reading it
        // agrees on them, and the next save keeps them.
        writeAll(file, R"({"bookmarks":[{"name":"F","children":[{"name":"","path":"/tmp"}]}]})");
        Bookmarks one(file, home.path());
        Bookmarks two(file, home.path());
        QCOMPARE(one.items(), two.items());
        const QString tmp = one.find("/tmp");
        QVERIFY(!tmp.isEmpty());
        QCOMPARE(one.get(tmp).value("name").toString(), QString("tmp"));
        QVERIFY(!one.add({}, "B", "/b").isEmpty());
        QVERIFY(QTest::qWaitFor([&] { return two.items().size() == 2; }, 3000));
        QCOMPARE(two.get(tmp).value("path").toString(), QString("/tmp"));
        const QJsonArray saved = QJsonDocument::fromJson(readAll(file)).object().value("bookmarks").toArray();
        QCOMPARE(saved.at(0).toObject().value("children").toArray().at(0).toObject().value("id").toString(), tmp);
    }

    void aFailedSaveShowsWhatsOnDisk()
    {
        QTemporaryDir home;
        const QString dir = home.filePath("cfg");
        const QString file = dir + "/bookmarks.json";
        Bookmarks b(file, home.path());
        const QString a = b.add({}, "A", "/a");
        QVERIFY(!a.isEmpty());
        const QByteArray before = readAll(file);

        struct Restore {
            QString dir;
            ~Restore() { QFile::setPermissions(dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner); }
        } restore{dir};
        QFile::setPermissions(dir, QFile::ReadOwner | QFile::ExeOwner);
        if (QFileInfo(dir).isWritable())
            QSKIP("Permissions don't apply (running as root?)");

        // What the bar would rebuild from at each change.
        QVariantList shown;
        int changes = 0;
        connect(&b, &Bookmarks::changed, this, [&] {
            shown = b.items();
            ++changes;
        });
        QVERIFY(b.add({}, "B", "/b").isEmpty());
        QCOMPARE(changes, 1);
        QCOMPARE(bookmarkNames(shown), QStringList{"A"});
        QVERIFY(!b.problem().isEmpty());
        QVERIFY(!b.edit(a, "Renamed", {}, {}).isEmpty());
        QCOMPARE(changes, 2);
        QCOMPARE(bookmarkNames(shown), QStringList{"A"});
        QCOMPARE(readAll(file), before);

        QFile::setPermissions(dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        QVERIFY(!b.add({}, "B", "/b").isEmpty());
        QVERIFY(b.problem().isEmpty());
        QCOMPARE(bookmarkNames(shown), (QStringList{"A", "B"}));
    }

    void editIsAllOrNothing()
    {
        QTemporaryDir home;
        const QString file = home.filePath("bookmarks.json");
        Bookmarks b(file, home.path());
        const QString f = b.addFolder({}, "F");
        const QString sub = b.addFolder(f, "Sub");
        const QString a = b.add({}, "A", "/a");
        QVERIFY(b.isWithin(sub, f));
        QVERIFY(b.isWithin(f, f));
        QVERIFY(b.isWithin(sub, {}));
        QVERIFY(!b.isWithin(f, sub));
        QVERIFY(!b.isWithin(a, f));

        // A folder into its own subfolder: nothing of the edit is saved.
        const QByteArray before = readAll(file);
        QCOMPARE(b.edit(f, "Renamed", {}, sub), QString("A folder can't go inside itself."));
        QCOMPARE(b.edit(f, "Renamed", {}, f), QString("A folder can't go inside itself."));
        QCOMPARE(readAll(file), before);
        QCOMPARE(b.get(f).value("name").toString(), QString("F"));
        QVERIFY(!b.edit("nothing", "X", {}, {}).isEmpty());

        // Renamed, pointed elsewhere and moved in one save.
        QSignalSpy changed(&b, &Bookmarks::changed);
        QCOMPARE(b.edit(a, "Desk", "/elsewhere", sub), QString());
        QCOMPARE(changed.count(), 1);
        const QVariantMap desk = Bookmarks(file, home.path()).get(a);
        QCOMPARE(desk.value("name").toString(), QString("Desk"));
        QCOMPARE(desk.value("path").toString(), QString("/elsewhere"));
        QCOMPARE(desk.value("parentId").toString(), sub);
        // Already there: stays where it is in the folder.
        QVERIFY(!b.add(sub, "B", "/b").isEmpty());
        QCOMPARE(b.edit(a, {}, {}, sub), QString());
        QCOMPARE(bookmarkNames(b.get(sub).value("children").toList()), (QStringList{"Desk", "B"}));
    }
};

QTEST_GUILESS_MAIN(BookmarksTests)
#include "bookmarks_tests.moc"
