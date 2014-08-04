TurbografxControllerToUSB
=========================

Simple adapter that transforms a Turbografx controller into a USB controller.

Turbografx controller pinout
============================

pin |        name        | comment
----|--------------------|---------
 1  | +5V                |
 2  | Up/I button        |
 3  | Right/II button    |
 4  | Down/Select button |
 5  | Left/Run button    |
 6  | Data select        |
 7  | ~Output Enable~    |
 8  | GND                |


Data select |   pin 2  |   pin 3   |     pin 4     |     pin 5
------------|----------|-----------|---------------|------------
 High       | Up       | Right     | Down          | Left
 Low        | I button | II button | Select button | Run button
