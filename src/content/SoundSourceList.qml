import QtQuick 2.0

import QtQml.Models 2.2
QtObject
{
    property list<TestSound> sounds: [
        TestSound {
            attenuationModel: {
                name:"attenuation_model"
                start: 20
                end: 180
            }
            sample: {
                name:"the_sample"
                source: "whistle.wav"
            }
            sound: {
                name:"the_sound"
                attenuationModel :"attenuation_model"
                category:"sound_sources"
                //PlayVariation {
                //     looping:true
                //     sample:"engine"
                //     maxGain:1.
                //     minGain:0.
               // }
             }
        }
    ]
}
