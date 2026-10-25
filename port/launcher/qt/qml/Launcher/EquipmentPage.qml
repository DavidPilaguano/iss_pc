// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import Launcher

Flickable {
    clip: true
    contentHeight: body.height + 52
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { }
    Column {
        id: body
        x: 28; y: 26; width: parent.width - 56; spacing: 16
        Text { text: "Mi equipo"; color: Theme.text; font.family: Theme.fontDisplay; font.pixelSize: 42 }
        Text { width: parent.width; text: "Especificaciones detectadas en esta computadora"; color: Theme.textMuted; font.family: Theme.fontText; font.pixelSize: 17; wrapMode: Text.WordWrap }
        Text { visible: Support.collecting; text: "Detectando equipo…"; color: Theme.cyan; font.pixelSize: 16 }
        Repeater {
            model: Support.hwRows
            Rectangle {
                width: body.width; height: row.implicitHeight + 28
                radius: 10; color: Theme.card; border.color: Theme.cardLine
                Column {
                    id: row
                    x: 16; y: 14; width: parent.width - 32; spacing: 5
                    Text { text: modelData.label; color: Theme.textMuted; font.family: Theme.fontText; font.pixelSize: 14 }
                    Text { width: parent.width; text: modelData.value; color: Theme.text; font.family: Theme.fontText; font.pixelSize: 18; wrapMode: Text.WordWrap }
                }
            }
        }
        GhostButton { text: "Actualizar detección"; enabled: !Support.collecting; onClicked: Support.collect() }
        Text { width: parent.width; text: "Los datos se consultan localmente. La VRAM y la RAM son memorias distintas. El rendimiento depende de la escena; 30 FPS con 8 GB sigue siendo el objetivo de optimización."; color: Theme.textMuted; font.family: Theme.fontText; font.pixelSize: 15; wrapMode: Text.WordWrap }
    }
}
