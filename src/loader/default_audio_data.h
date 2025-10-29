#pragma once

// Use a namespace to avoid name conflicts
namespace DefaultAudioData
{
    // We use a C++11 raw string literal (R"()")
    // This allows pasting multi-line text easily
    const char* GtaSaVehicleAudioSettings = R"(
; ========================================================================
;
;   gtasa_vehicleAudioSettings.cfg
;
;   ! ! ! IMPORTANT ! ! !
;   PASTE THE CONTENTS OF YOUR CLEAN (DEFAULT)
;   gtasa_vehicleAudioSettings.cfg FILE HERE
;
;   The text below is just an example, not the full file!
;
; ========================================================================

;-- Bank 0 -------------------------------------------
; VEHICLE_AUDIO_PETROL_BANK_0_TWIN_EXHAUST
10_0    0 0 11 1 0.400000 0.850000 0 1.000000 1 0 1 0 1 0.000000
10_1    0 1 11 1 0.400000 0.850000 0 1.000000 1 0 1 0 1 0.000000
10_2    0 2 11 1 0.400000 0.850000 0 1.000000 1 0 1 0 1 0.000000
10_3    0 3 11 1 0.400000 0.850000 0 1.000000 1 0 1 0 1 0.000000

; ...etc...

;the end
)";
}