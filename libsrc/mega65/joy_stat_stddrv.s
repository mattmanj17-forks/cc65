;
; Address of the static standard joystick driver
;
; Oliver Schmidt, 2012-11-01
;
; const void joy_static_stddrv[];
;

        .export _joy_static_stddrv
        .import _mega65_stdjoy_joy

.rodata

_joy_static_stddrv := _mega65_stdjoy_joy
