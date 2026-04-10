# Gas-Leakage-System
A safety-critical embedded system capable of detecting hazardous gas concentrations and executing automated physical countermeasures (ventilation and window actuation).

Key Features & Modules:

    Gas Sensing & Alarm: Integrated an MQ-series gas sensor with an Arduino-based logic controller to trigger real-time audio-visual alerts via an LCD 1602A and a Passive Buzzer.

    Automated Actuation: Developed a window-control mechanism using a ULN2003A Driver and a Stepper Motor, complemented by a high-speed DC Motor for forced ventilation.

    Remote Control & Safety: Implemented an Infrared (IR) control interface (VS1838B) for manual system overriding and emergency stops.

    Hardware Design: Engineered the full circuit schematics in KiCad, ensuring signal integrity and proper power distribution using PC817 Optocouplers for galvanic isolation between the logic and power stages.

Technical skills:

    Hardware: Arduino, KiCad (PCB Design), Stepper & DC Motors, IR Sensors, Optocouplers.

    Software: Embedded C++, Arduino IDE.
