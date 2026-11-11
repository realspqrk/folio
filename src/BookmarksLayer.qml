import QtQuick
import QtQuick.Controls
import QtCore

// Everything of the bookmarks bar a window holds, in one place: the bar
// under the path bars, the star's bubble, the editor and its question, and
// the keys (Ctrl+B, Ctrl+Shift+B, F6). Main.qml lays it over the window and
// makes the bar's room below the path (barHeight); the panes ask it whether
// keys are going into it (busy) and to bookmark (bookmark()).
Item {
    id: layer

    // The window: its panes, and whether keys are going into a field.
    readonly property var win: ApplicationWindow.window
    // Where the bar sits: under the path bars.
    property int barTop: 0

    // The bar shown (Ctrl+Shift+B), the room it takes, and the folder the
    // star last put a bookmark in, as a browser remembers.
    property bool barVisible: true
    readonly property int barHeight: barVisible ? bar.implicitHeight : 0
    property string lastFolder: ""
    // A bookmark dialog, the bubble, or the keyboard on the bar (F6): the
    // panes' single-key shortcuts stand aside.
    readonly property bool busy: dialog.visible || confirm.visible || bubble.visible || bar.activeFocus

    readonly property alias bar: bookmarksBar
    readonly property alias bubble: bookmarkBubble
    readonly property alias dialog: bookmarkDialog
    readonly property alias confirm: bookmarkConfirm

    // Kept beside the window's own settings.
    Settings {
        property alias bookmarksBarVisible: layer.barVisible
        property alias lastBookmarkFolder: layer.lastFolder
    }

    // Hidden with the keys on it (any way: shortcut, its own menu): they go
    // to the files.
    onBarVisibleChanged: if (!barVisible && bar.activeFocus) focusFiles()
    function focusFiles() { if (win && win.activePane) win.activePane.focusView() }

    // Ctrl+B or the star: bookmarks a path right away and opens the bubble
    // to name it, or edits its bookmark when it has one, as a browser does.
    function bookmark(path, anchor) {
        const pane = win ? win.activePane : null
        if (!bubble.openFor(path, anchor || (pane ? pane.bookmarkStar : null)) && pane)
            pane.paneBackend.showMessage(bookmarks.problem.length > 0 ? bookmarks.problem
                                                                      : qsTr("Couldn't bookmark %1.").arg(path))
    }
    // A folder of bookmarks asks first; a bookmark just goes.
    function deleteBookmark(id) {
        const item = bookmarks.get(id)
        if (item.type === "folder" && item.children.length > 0)
            confirm.ask(qsTr("Delete this folder?"),
                        qsTr("%1 and the %Ln bookmarks and folders in it will be deleted.", "", item.children.length).arg(item.name),
                        qsTr("Delete"), () => bookmarks.remove(id))
        else
            bookmarks.remove(id)
    }

    // Also with the keys on the bar (F6), as a browser's bookmark keys work
    // from its toolbar.
    Shortcut {
        sequence: "Ctrl+B"
        enabled: !!layer.win && (!layer.win.typing || layer.bar.activeFocus) && !!layer.win.activePane
        onActivated: layer.bookmark(layer.win.activePane.paneBackend.path)
    }
    Shortcut {
        sequence: "Ctrl+Shift+B"
        enabled: !!layer.win && (!layer.win.typing || layer.bar.activeFocus)
        onActivated: layer.barVisible = !layer.barVisible
    }
    // F6, as in a browser: the keyboard to the bookmarks bar.
    Shortcut {
        sequence: "F6"
        enabled: !!layer.win && !layer.win.typing && layer.barVisible
        onActivated: layer.bar.focusBar()
    }

    // Across the whole window, under the path bars, over the panes' room for
    // it (FolderPane leaves barHeight free).
    BookmarksBar {
        id: bookmarksBar
        y: layer.barTop
        width: layer.width
        visible: layer.barVisible
        target: layer.win && layer.win.activePane ? layer.win.activePane.paneBackend : null
        targetFileOps: layer.win && layer.win.activePane ? layer.win.activePane.paneFileOps : null
        onAddRequested: (path, parentId, index) => bookmarkDialog.add(path, parentId, index)
        onEditRequested: (id) => bookmarkDialog.edit(id)
        onNewFolderRequested: (parentId, index) => bookmarkDialog.newFolder(parentId, index)
        onDeleteRequested: (id) => layer.deleteBookmark(id)
        onHideRequested: layer.barVisible = false
        onDone: layer.focusFiles()
        quiet: bookmarkDialog.visible || bookmarkConfirm.visible || bookmarkBubble.visible
    }

    BookmarkDialog {
        id: bookmarkDialog
        anchors.fill: parent
        completer: layer.win && layer.win.activePane ? layer.win.activePane.paneBackend : null
        onVisibleChanged: if (!visible) layer.focusFiles()
    }
    BookmarkBubble {
        id: bookmarkBubble
        parent: Overlay.overlay
        lastFolder: layer.lastFolder
        onFolderUsed: (id) => layer.lastFolder = id
        onClosed: layer.focusFiles()
    }
    ConfirmDialog {
        id: bookmarkConfirm
        anchors.fill: parent
        onVisibleChanged: if (!visible) layer.focusFiles()
    }
}
