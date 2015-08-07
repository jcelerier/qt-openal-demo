import QtQuick 2.0
import QtAudioEngine 1.0

QtObject {
    property string name: "FirstSource"
    property AttenuationModelLinear attenuationModel

    property AudioSample sample
    property Sound sound
}
