TurbografxControllerToUSB
=========================

Simple adapter that transforms a Turbografx controller into a USB controller.

Turbografx controller pinout
============================

pin |      name       | comment
----|-----------------|---------
 1  | +5V             |
 2  | Data0           |
 3  | Data1           |
 4  | Data2           |
 5  | Data3           |
 6  | Data select     |
 7  | ~Output Enable~ | Also used as clock for auto-fire
 8  | GND             |


Data select |   Data0    |   Data1   |     Data2     |    Data3
------------|------------|-----------|---------------|------------
 High       | Up         | Right     | Down          | Left
 Low        | I button   | II button | Select button | Run button
 High       | 0          | 0         | 0             | 0
 Low        | III button | IV button | V button      | VI button
