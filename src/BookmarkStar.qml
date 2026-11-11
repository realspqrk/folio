import QtQuick
import QtQuick.Shapes

// The path bar's star: an outline, filled when the folder is bookmarked, as
// in a browser. Drawn like Glyph's line icons (same grid, stroke and caps),
// which have no filled shapes of their own.
Item {
    id: star

    property bool filled: false
    property color color: theme.foreground
    property real size: 18

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: star.color
            fillColor: star.filled ? star.color : "transparent"
            strokeWidth: 1.6
            capStyle: theme.radius > 0 ? ShapePath.RoundCap : ShapePath.SquareCap
            joinStyle: theme.radius > 0 ? ShapePath.RoundJoin : ShapePath.MiterJoin
            scale: Qt.size(star.size / 24, star.size / 24)
            PathSvg { path: "M12 3l2.7 5.6 6.1.9-4.4 4.3 1 6.1L12 17l-5.4 2.9 1-6.1-4.4-4.3 6.1-.9z" }
        }
    }
}
