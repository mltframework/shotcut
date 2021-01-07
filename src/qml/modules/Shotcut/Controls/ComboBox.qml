import QtQuick.Controls 2.12

// workaround incorrect color of chosen combo item on Fusion theme
ComboBox {
    palette.buttonText: palette.buttonText
}
