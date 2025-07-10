#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "gCodeInputAndFormatting.h" // Utility functions for input validation and G-Code formatting

struct ODHelixGroove {
    // user input variables
    double helixAngleDeg;
    double normalGrooveWidth;
    double lead;
    int numStarts;
    char handedness;
    double majorOD;
    double grooveDepth;
    int numDepthPasses;
    double xStart;
    double xEnd;
    double cutterDiameter;
    double edgebreakRadius;
    int numFlutes;
    int sfpm;
    double ipt;
    double xClear;

    const double PI = 3.1416; // PI with 4 significant digits for gcode math
    const double EPSILON = 1e-4; // Used to compare floating-point values, accounts for rounding error due to binary representation limits. e.g. use if(abs(currentZ - roughFloorZ) < EPSILON) instead of if(currentZ == roughFloorZ)

    std::string inputLog;
    std::ostringstream gCodeOutput;

    // NOTE: on numeric userInput...() calls, you can pass true as an argument to allow 'mm' suffix - see "gCodeInputAndFormatting.h"
    void promptInput() {
        int state = 0;
        while (state < 17) {
            try {
                switch (state) {
                case 0: {
                    std::cout << "Enter Helix Angle (degrees): ";
                    helixAngleDeg = userInputPos();
                    helixAngleDeg = roundFourDecimal(helixAngleDeg);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(helixAngleDeg) << "\n";
                    if (helixAngleDeg >= 90 || helixAngleDeg < EPSILON) {
                        std::cout << "Must be between 0 and 90 degrees. 90 is parallel to the x axis.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 1: {
                    std::cout << "Enter Normal Groove Width (inches, aka ideal tool width for full slotting): ";
                    normalGrooveWidth = userInputPos(true);
                    normalGrooveWidth = roundFourDecimal(normalGrooveWidth);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(normalGrooveWidth) << "\n";
                    if (normalGrooveWidth > 1) {
                        std::cout << "Is it that wide? Might want to double check that.\n";
                    }
                    if (normalGrooveWidth < .03) {
                        std::cout << "Surely it's not that small. Might want to double check that.\n";
                    }
                    if (normalGrooveWidth < .0001) {
                        std::cout << "Enter a bigger number.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 2: {
                    std::cout << "Enter Lead (inches, Axial Pitch * Number of Starts, aka axial distance for a single lead to complete 1 rev): ";
                    lead = userInputPos(true);
                    lead = roundFourDecimal(lead);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(lead) << "\n";
                    std::cout << std::endl;
                    break;
                }
                case 3: {
                    std::cout << "Enter Number of Starts: ";
                    numStarts = userInputPos();
                    numStarts = static_cast<int>(round(numStarts));
                    std::cout << "You entered: " << numStarts << "\n";
                    if (numStarts > 100) {
                        std::cout << "Might want to double check that.\n";
                    }
                    if (numStarts < 1) {
                        std::cout << "Must be at least 1.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 4: {
                    std::cout << "Enter Handedness (L for left-hand, R for right-hand): ";
                    handedness = userInputLR();
                    std::string handednessConfirmation = (handedness == 'l' || handedness == 'L') ? "Left" : "Right";
                    std::cout << "You entered: " << handednessConfirmation << "\n";
                    std::cout << std::endl;
                    break;
                }
                case 5: {
                    std::cout << "Enter Major OD (inches): ";
                    majorOD = userInputPos(true);
                    majorOD = roundFourDecimal(majorOD);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(majorOD) << "\n";
                    if (majorOD > 12) {
                        std::cout << "Will that fit in the machine? Might want to double check that.\n";
                    }
                    if (majorOD < 1) {
                        std::cout << "Surely it's not that small. Might want to double check that.\n";
                    }
                    std::cout << std::endl;
                    break;
                }
                case 6: {
                    std::cout << "Enter Groove Depth (inches): ";
                    grooveDepth = userInputPosNeg(true);
                    grooveDepth = abs(roundFourDecimal(grooveDepth));
                    std::cout << "You entered: " << formatGCodeDecimals<4>(grooveDepth) << "\n";
                    if (grooveDepth > .5) {
                        std::cout << "Looks deep. Might want to double check that.\n";
                    }
                    std::cout << std::endl;
                    break;
                }
                case 7: {
                    std::cout << "Enter Number of Depth Passes: ";
                    numDepthPasses = userInputPos();
                    numDepthPasses = static_cast<int>(round(numDepthPasses));
                    std::cout << "You entered: " << numDepthPasses << " passes which is " << formatGCodeDecimals<4>(roundFourDecimal(grooveDepth / numDepthPasses)) << " per pass\n";
                    if (numDepthPasses > 100) {
                        std::cout << "Too many.\n";
                        continue;
                    }
                    if (numDepthPasses < 1) {
                        std::cout << "Must be at least 1.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 8: {
                    std::cout << "Enter X-axis Feature Start Location (inches, x-coordinate of left face of feature relative to x-origin): ";
                    xStart = userInputPosNeg(true);
                    xStart = roundFourDecimal(xStart);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xStart) << "\n";
                    std::cout << std::endl;
                    break;
                }
                case 9: {
                    std::cout << "Enter X-axis Feature End Location (inches, x-coordinate of right face of feature relative to x-origin): ";
                    xEnd = userInputPosNeg(true);
                    xEnd = roundFourDecimal(xEnd);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xEnd) << "\n";
                    if (xEnd <= xStart) {
                        std::cout << "The X Endpoint must be to the right of the X Start point.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 10: {
                    std::cout << "Enter Cutter Diameter (inches): ";
                    cutterDiameter = userInputPos(true);
                    cutterDiameter = roundFourDecimal(cutterDiameter);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(cutterDiameter) << "\n";
                    if (cutterDiameter > normalGrooveWidth) {
                        std::cout << "Enter a tool that's the same width as the groove or smaller.\n";
                        continue;
                    }
                    if (cutterDiameter < normalGrooveWidth / 2) {
                        std::cout << "Enter a tool that's at least half the groove width.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 11: {
                    std::cout << "Enter Desired Edgebreak Radius (inches): ";
                    edgebreakRadius = userInputPosZero(true);
                    edgebreakRadius = roundFourDecimal(edgebreakRadius);
                    double circumferentialGrooveWidth = roundFourDecimal(normalGrooveWidth / std::sin(helixAngleDeg * PI / 180.0));
                    double circumferentialVaneWidth = roundFourDecimal(((majorOD * PI) / numStarts) - circumferentialGrooveWidth);
                    double normalVaneWidth = roundFourDecimal(circumferentialVaneWidth * std::sin(helixAngleDeg * PI / 180.0));
                    double circumferentialVaneRemovalFromFilletCut = roundFourDecimal(edgebreakRadius / std::tan((helixAngleDeg / 2.0) * PI / 180.0));
                    double circumferentialGrooveReachFromFilletArcCenter = roundFourDecimal(std::sqrt(std::pow(edgebreakRadius + (cutterDiameter / 2.0), 2) - std::pow(edgebreakRadius, 2)));
                    double circumferentialGrooveRemovalFromSharp = roundFourDecimal(circumferentialGrooveReachFromFilletArcCenter + (cutterDiameter / 2.0) - circumferentialVaneRemovalFromFilletCut);
                    if (edgebreakRadius < EPSILON) {
                        edgebreakRadius = 0;
                    }
                    std::cout << "You entered: " << formatGCodeDecimals<4>(edgebreakRadius) << "\n";
                    // Check for wiping out the vane with an oversize fillet
                    if (edgebreakRadius > (roundFourDecimal(normalVaneWidth / 2))) {
                        std::cout << "\n[WARNING] The normal vane width (remaining material) is " << formatGCodeDecimals<4>(normalVaneWidth) << " wide.\n";
                        std::cout << "[WARNING] The edgebreak radius cannot exceed half of that or R" << formatGCodeDecimals<4>(roundFourDecimal(normalVaneWidth / 2)) << "\n";
                        std::cout << "Press Enter to retry...";
                        while (true) {
                            if (_kbhit()) {
                                char ch = _getch();
                                if (ch == ENTER) {
                                    std::cout << std::endl << std::endl;
                                    break;
                                }
                            }
                        }
                        continue;
                    }
                    else if (edgebreakRadius < EPSILON) {
                        std::cout << "You chose not to edgebreak. Tool will not wrap around face of feature.\n";
                        std::cout << std::endl;
                    }
                    // Inform user of how much circumferential vane width there is and how much will be cut away
                    else {
                        std::cout << "\nNote: The normal vane thickness is " << formatGCodeDecimals<4>(normalVaneWidth) << " wide.\n";
                        std::cout << "[CAUTION]: The circumferential vane width (aka the vane width when cross-sectioned at " << formatGCodeDecimals<4>(helixAngleDeg) << " deg) is " << formatGCodeDecimals<4>(circumferentialVaneWidth) << "\n";
                        std::cout << "[CAUTION]: An edgebreak radius of " << formatGCodeDecimals<4>(edgebreakRadius) << " will remove " << formatGCodeDecimals<4>(circumferentialVaneRemovalFromFilletCut) << " from the circumferential vane width at the exposed face\n";
                        if ((circumferentialVaneRemovalFromFilletCut / std::cos((helixAngleDeg / 2.0) * PI / 180.0)) > cutterDiameter) {
                            std::cout << "[WARNING]: This fillet and tool combination will leave an uncut island.\n";
                            std::cout << "Choose a larger tool or a smaller fillet.\n";
                            std::cout << "Press Enter to retry...";
                            while (true) {
                                if (_kbhit()) {
                                    char ch = _getch();
                                    if (ch == ENTER) {
                                        std::cout << std::endl << std::endl;
                                        break;
                                    }
                                }
                            }
                            continue;
                        }
                        else {
                            std::cout << "Press Enter to continue...";
                            while (true) {
                                if (_kbhit()) {
                                    char ch = _getch();
                                    if (ch == ENTER) {
                                        std::cout << std::endl << std::endl;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    /* !!!!!Removing because nigh impossible to get a tool that won't leave an island b/c with X+Rotary moves we are cutting at an angle i.e. with an x/y arc we're tring to bridge from the arc to the circumferential groove width with the tool that cut a normal groove width (as opposed to axial like what we do with the fillet) therefore we'll change the cut strategies - so let's do always straight with no fillet, then come back and plunge at fillet location and cut only the arc to edgebreak in an op fully separate from main groove
                    // Check for uncut island
                    if (((circumferentialGrooveWidth / 2.0) > circumferentialGrooveRemovalFromSharp) && (edgebreakRadius > EPSILON)) {
                        std::cout << "\n[WARNING]: This fillet and tool combination will leave an uncut island.\n";
                        // Estimate minimum required tool radius to avoid island
                        double minToolRadius = std::sqrt(std::pow(((circumferentialGrooveWidth / 2.0) + circumferentialVaneRemovalFromFilletCut), 2) + std::pow(edgebreakRadius, 2)) - edgebreakRadius;
                        double minToolDiameter = minToolRadius * 2.0 * std::sin(helixAngleDeg * PI / 180.0);
                        std::cout << "Minimum required tool diameter to avoid island: " << formatGCodeDecimals<4>(roundFourDecimal(minToolDiameter)) << " inches\n";
                        std::cout << "Consider:\n";
                        std::cout << " - Increasing tool size to at least " << formatGCodeDecimals<4>(roundFourDecimal(minToolDiameter)) << " inches diameter\n";
                        std::cout << " - Reducing fillet radius below current R" << formatGCodeDecimals<4>(edgebreakRadius) << "\n";
                        std::cout << " - Or generating a second gcode file on the groove centerline without fillets to remove the island\n";
                        std::cout << "Press Enter to continue...";
                        while (true) {
                            if (_kbhit()) {
                                char ch = _getch();
                                if (ch == ENTER) {
                                    std::cout << std::endl; // new line in end user command line interface
                                    break;
                                }
                            }
                        }
                    }
                    */
                    //std::cout << std::endl;
                    break;
                }
                case 12: {
                    std::cout << "Enter Number of Flutes on Endmill: ";
                    numFlutes = userInputPos();
                    numFlutes = static_cast<int>(round(numFlutes));
                    std::cout << "You entered: " << numFlutes << "\n";
                    if (numFlutes > 10) {
                        std::cout << "I don't think it has that many flutes.\n";
                        continue;
                    }
                    if (numFlutes < 2) {
                        std::cout << "Pick a tool with more cutting edges.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 13: {
                    std::cout << "Enter Cutter Speed (sfpm): ";
                    sfpm = userInputPos();
                    sfpm = static_cast<int>(round(sfpm));
                    std::cout << "You entered: " << sfpm << " SFPM which is " << round(sfpm / (PI * (cutterDiameter / 12))) << " RPM\n";
                    if (sfpm > 1000) {
                        std::cout << "A bit fast. Please enter a more reasonable value.\n";
                        continue;
                    }
                    if (sfpm < 20) {
                        std::cout << "A bit slow. Please enter a more reasonable value.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 14: {
                    std::cout << "Enter Chipload (ipt): ";
                    ipt = userInputPos(true);
                    ipt = roundFourDecimal(ipt);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(ipt) << " IPT which is " << formatGCodeDecimals<3>(roundThreeDecimal(round(sfpm / (PI * (cutterDiameter / 12))) * ipt * numFlutes)) << " IPM\n";
                    if (ipt > 0.009) {
                        std::cout << "Feed per tooth too high. Please enter a more reasonable value.\n";
                        continue;
                    }
                    if (ipt < 0.0001) {
                        std::cout << "Feed per tooth too low. Please enter a more reasonable value.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 15: {
                    std::cout << "Enter Clearance Gap in X: ";
                    xClear = userInputPos(true);
                    xClear = roundFourDecimal(xClear);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xClear) << "\n";
                    if (xClear < 0.005) {
                        std::cout << "Enter at least .005 clearance.\n";
                        continue;
                    }
                    if (xClear >= (cutterDiameter / 2)) {
                        std::cout << "Note: You do not have to account for half the tool diameter.\n";
                    }
                    std::cout << std::endl;
                    break;
                }
                case 16: {
                    std::cout << "Press Enter to generate gcode (or Escape to go back): ";
                    while (true) {
                        if (_kbhit()) {
                            char ch = _getch();
                            if (ch == ESC) throw std::runtime_error("EscapePressed");
                            if (ch == ENTER) break;
                        }
                    }
                    break;
                }
                }
            }
            catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "EscapePressed") {
                    if (state > 0) {
                        std::cout << "\n[Esc Pressed. Going Back One Level...]\n\n";
                        state--;
                    }
                    if (state == 0) {
                        std::cout << "\n";
                    }
                    continue;
                }
                else {
                    std::cerr << "[Unhandled exception in input state " << state << "]: " << e.what() << "\n";
                    throw;
                }
            }
            state++;
        }
    }

    void logInputs() {
        // prompt labels strictly for the input log.txt string
        const char* promptLabel[16] = {
            "Helix Angle (degrees)",
            "Normal Groove Width (inches)",
            "Lead (inches)",
            "Number of Starts",
            "Handedness",
            "Major OD (inches)",
            "Groove Depth (inches)",
            "Number of Depth Passes",
            "X-axis Start Position (inches, left face of feature)",
            "X-axis End Position (inches, right face of feature)",
            "Cutter Diameter (inches)",
            "Edgebreak Radius (inches)",
            "Number of Flutes",
            "Cutter Speed (sfpm)",
            "Feedrate (ipt)",
            "Clearance Gap in X"
        };
        // Append all prompt labels and final user inputs to the input log
        for (int i = 0; i < 16; ++i) {
            std::string value;
            switch (i) {
            case 0:  value = formatGCodeDecimals<4>(helixAngleDeg); break;
            case 1:  value = formatGCodeDecimals<4>(normalGrooveWidth); break;
            case 2:  value = formatGCodeDecimals<4>(lead); break;
            case 3:  value = std::to_string(numStarts); break;
            case 4:  value = (handedness == 'l' || handedness == 'L') ? "Left" : "Right"; break;
            case 5:  value = formatGCodeDecimals<4>(majorOD); break;
            case 6:  value = formatGCodeDecimals<4>(grooveDepth); break;
            case 7:  value = std::to_string(numDepthPasses); break;
            case 8:  value = formatGCodeDecimals<4>(xStart); break;
            case 9:  value = formatGCodeDecimals<4>(xEnd); break;
            case 10: value = formatGCodeDecimals<4>(cutterDiameter); break;
            case 11: value = formatGCodeDecimals<4>(edgebreakRadius); break;
            case 12: value = std::to_string(numFlutes); break;
            case 13: value = std::to_string(sfpm); break;
            case 14: value = formatGCodeDecimals<4>(ipt); break;
            case 15: value = formatGCodeDecimals<4>(xClear); break;
            }
            inputLog += std::string(promptLabel[i]) + ": " + value + "\n";
        }
    }

    void generateGCode() {
        std::ostringstream g;

        int rpm = static_cast<int>(round(sfpm / ((cutterDiameter / 12) * PI)));
        double ipm = roundThreeDecimal(rpm * ipt * numFlutes);
        double ipmOneIPR = roundThreeDecimal(ipm / (1000.0 * numFlutes * ipt));

        double xFeatureLength = std::abs(xEnd - xStart);
        double toolClear = roundFourDecimal((cutterDiameter / 2.0) + xClear);
        double xTravelFull = xFeatureLength + 2 * toolClear;
        double degreesPerInch = 360.0 / lead;
        double bTravelFull = roundThreeDecimal(degreesPerInch * xTravelFull);
        double yOffsetToWall = roundFourDecimal((normalGrooveWidth - cutterDiameter) / (2.0 * std::sin(helixAngleDeg * PI / 180.0)));

        double offsetX = roundFourDecimal((edgebreakRadius + cutterDiameter / 2.0) * std::cos(helixAngleDeg * PI / 180.0));
        double offsetY = roundFourDecimal((edgebreakRadius + cutterDiameter / 2.0) * std::sin(helixAngleDeg * PI / 180.0));

        double bIndex = roundThreeDecimal(360.0 / numStarts);
        double depthPerPass = roundFourDecimal(grooveDepth / numDepthPasses);

        std::string printHandedness;
        bool isRight;
        if (handedness == 'R' || handedness == 'r') {
            printHandedness = "RIGHT";
            isRight = true;
        }
        else {
            printHandedness = "LEFT";
            isRight = false;
        }

        // N2 variables - incremental G91 moves - cut grooves
        double forwardX = xTravelFull;
        double returnX = -xTravelFull;
        double forwardB = isRight ? bTravelFull : -bTravelFull;
        double returnB = isRight ? -bTravelFull : bTravelFull;
        double forwardYOffset = -yOffsetToWall;
        double returnYOffset = yOffsetToWall;

        // N3 variables - cut fillets
        // R word = fillet rad + tool rad
        double radiusFilletFull = roundFourDecimal(edgebreakRadius + (cutterDiameter / 2.0));
        // absolute G90 xy moves right end
        double xFilletRightStart = roundFourDecimal(xStart + xFeatureLength - edgebreakRadius - (std::cos(helixAngleDeg * PI / 180.0) * (edgebreakRadius + (cutterDiameter / 2.0))));
        double yFilletRightStart = (0 - yOffsetToWall);
        double xFilletRightEnd = roundFourDecimal(xStart + xFeatureLength + (cutterDiameter / 2.0));
        double yFilletRightEnd = roundFourDecimal(yFilletRightStart - (std::sin(helixAngleDeg * PI / 180.0) * (edgebreakRadius + (cutterDiameter / 2.0))));
        // absolute G90 xy moves left end
        double xFilletLeftStart = roundFourDecimal(xStart + edgebreakRadius + (std::cos(helixAngleDeg * PI / 180.0) * (edgebreakRadius + (cutterDiameter / 2.0))));
        double yFilletLeftStart = (0 + yOffsetToWall);
        double xFilletLeftEnd = roundFourDecimal(xStart - (cutterDiameter / 2.0));
        double yFilletLeftEnd = roundFourDecimal(yFilletLeftStart + (std::sin(helixAngleDeg * PI / 180.0) * (edgebreakRadius + (cutterDiameter / 2.0))));
        // absolute G90 b initial
        double bFilletInitialRight = std::abs(roundThreeDecimal(degreesPerInch * (xFilletRightStart + toolClear)));
        // incremental G91 b moves
        double xFilletStartDelta = std::abs(xFilletRightStart - xFilletLeftStart);
        double bFilletMoveLeft = roundThreeDecimal(degreesPerInch * xFilletStartDelta);
        double bFilletMoveRight = roundThreeDecimal(degreesPerInch * xFilletStartDelta);
        // sign handling for N3 b's
        bFilletInitialRight = isRight ? bFilletInitialRight : -bFilletInitialRight;
        bFilletMoveLeft = isRight ? -bFilletMoveLeft : bFilletMoveLeft;
        bFilletMoveRight = isRight ? bFilletMoveRight : -bFilletMoveRight;

        //////////////////////////
        // HEADER
        /////////////////////////
        g << "%\n";
        g << "O1 (DwgNo ISSUE -)\n";
        g << "(ProgrammerName Mo-Da-Year)\n";
        g << "(Material)\n";
        g << "(" << printHandedness << " HAND OD HELIX GROOVES)\n";
        g << "(PROGRAMMED FOR A CHUCK ON THE RIGHT END OF MACHINE TABLE)\n";
        g << "(VERIFY B AXIS ISN'T REVERSED BY MACHINE PARAMETERS)\n\n";

        g << "(OP 1)\n\n";

        g << "(BUTTON FIXTURE)\n";
        g << "(0.0000 OD OUT)\n";
        g << "(SET Z ON " << formatGCodeDecimals<4>(majorOD) << " OD)\n";
        g << "(SET X ON LEFT FACE)\n\n";

        g << "M0(EDIT HAAS SETTING 79 5TH AXIS DIAMETER TO THE MAJOR OD OF THE VANES " << formatGCodeDecimals<4>(majorOD) << " FOR FEEDRATE CALCULATION)\n\n";

        //////////////////////////
        // GROOVE OP
        /////////////////////////
        g << "T5 M06 (" << formatGCodeDecimals<4>(cutterDiameter) << " DIA ENDMILL " << numFlutes << "FL)\n";
        g << "(" << printHandedness << " HAND OD HELIX GROOVES " << formatGCodeDecimals<4>(normalGrooveWidth) << " WIDE " << formatGCodeDecimals<4>(grooveDepth) << " DEEP)\n";
        g << "(CUTTING SPEED : " << sfpm << " SFPM)\n";
        g << "(CHIP LOAD : " << formatGCodeDecimals<4>(ipt) << " IPT)\n";
        g << "(REMAINING STOCK : XY:0. | Z:0.)\n";
        g << "G0 G91 G28 Z0.\n";
        g << "G0 G91 G28 Y0.\n";
        g << "G0 G17 G90 G56 A-90. B0.\n";
        g << "G0 X-8. Y0. S" << rpm << " M03\n";
        g << "G0 G43 H05 Z5. M8\n";
        g << "G0 X" << formatGCodeDecimals<4>(xStart - toolClear) << " (" << formatGCodeDecimals<4>(xClear) << " CLEAR)\n";
        g << "G91 M97 P1 L" << numStarts << " B" << formatGCodeDecimals<3>(bIndex) << "\n";
        g << "G0 G90 Z5.\n";
        g << "X-8. M9\n";
        g << "G0 G91 G28 Z0.\n";
        g << "G0 G91 G28 Y0.\n";
        g << "M0\n\n";

        //////////////////////////
        // FILLET OP
        /////////////////////////
        if (edgebreakRadius > EPSILON) {
            g << "T5 M06 (" << formatGCodeDecimals<4>(cutterDiameter) << " DIA ENDMILL " << numFlutes << "FL)\n";
            g << "(" << printHandedness << " HAND OD HELIX GROOVES - R" << formatGCodeDecimals<4>(edgebreakRadius) << " EDGEBREAK)\n";
            g << "(CUTTING SPEED : " << sfpm << " SFPM)\n";
            g << "(CHIP LOAD : .001 IPR)\n";
            g << "(REMAINING STOCK : XY:0. | Z:0.)\n";
            g << "G0 G91 G28 Z0.\n";
            g << "G0 G91 G28 Y0.\n";
            g << "G0 G17 G90 G56 A-90. B" << formatGCodeDecimals<3>(bFilletInitialRight) << " (INITIAL B COORDINATE MAPPED TO RIGHT END FILLET START)\n";
            g << "G0 X-8. Y0. S" << rpm << " M03\n";
            g << "G0 G43 H05 Z5. M8\n";
            g << "G0 X" << formatGCodeDecimals<4>(xStart - toolClear) << " (" << formatGCodeDecimals<4>(xClear) << " CLEAR)\n";
            g << "G91 M97 P3 L" << numStarts << " B" << formatGCodeDecimals<3>(bIndex) << "\n";
            g << "G0 G90 Z5.\n";
            g << "X-8. M9\n";
            g << "G0 G91 G28 Z0.\n";
            g << "G0 G91 G28 Y0.\n";
            g << "M30\n\n";
        }

        //////////////////////////
        // N1 DEPTH PASSES SUBROUTINE
        /////////////////////////
        g << "N1 (OD HELIX GROOVE - DEPTH PASSES SUBROUTINE)\n";
        g << "G91 M97 P2 L" << numDepthPasses << " Z" << formatGCodeDecimals<4>(-depthPerPass) << "\n";
        g << "G0 G90 Z5.\n";
        g << "M99\n\n";

        //////////////////////////
        // N2 GROOVE SUBROUTINE - FIX Y, MOVE X/ROTARY
        /////////////////////////
        g << "N2 (OD HELIX GROOVE - CUT GROOVES SUBROUTINE)\n";
        g << "G0 G90 X" << formatGCodeDecimals<4>(xStart - toolClear) << " Y0.\n";
        g << "G0 G91 Z" << formatGCodeDecimals<4>(-5.0 + .1) << "\n";
        g << "G1 Z" << formatGCodeDecimals<4>(-depthPerPass - .1) << " F" << formatGCodeDecimals<3>(ipm) << " (" << formatGCodeDecimals<4>(ipt) << " IPT)\n";

        // cut strategy selection
        if (fabs(normalGrooveWidth - cutterDiameter) < EPSILON) {
            g << "(*****SINGLE PATH WITH ON-SIZE TOOL*****)\n";
            g << "G1 X" << formatGCodeDecimals<4>(forwardX) << " B" << formatGCodeDecimals<4>(forwardB) << "\n";
            g << "G0 Z5.\n";
            g << "G0 X" << formatGCodeDecimals<4>(returnX) << " B" << formatGCodeDecimals<4>(returnB) << "\n";
        }
        else {
            g << "(*****DUAL PATH WITH UNDERSIZED TOOL*****)\n";
            g << "(FORWARD PASS ON ONE WALL)\n";
            g << "G1 Y" << formatGCodeDecimals<4>(forwardYOffset) << "\n";
            g << "G1 X" << formatGCodeDecimals<4>(forwardX) << " B" << formatGCodeDecimals<4>(forwardB) << "\n";
            g << "(REVERSE PASS ON OPPOSITE WALL)\n";
            g << "G1 Y" << formatGCodeDecimals<4>(returnYOffset * 2.0) << "\n";
            g << "G1 X" << formatGCodeDecimals<4>(returnX) << " B" << formatGCodeDecimals<4>(returnB) << "\n";
            g << "G0 Z5.\n";
            g << "G0 G90 Y0.\n";
        }

        g << "M99\n";

        //////////////////////////
        // N3 FILLET SUBROUTINE - FIX ROTARY, MOVE X/Y
        /////////////////////////
        if (edgebreakRadius > EPSILON) {
            g << "\nN3 (OD HELIX GROOVE - R" << formatGCodeDecimals<4>(edgebreakRadius) << " EDGEBREAK SUBROUTINE)\n";
            g << "(*****RIGHT FILLET*****)\n";
            g << "G0 G90 X" << formatGCodeDecimals<4>(xFilletRightStart) << " Y" << formatGCodeDecimals<4>(yFilletRightStart) << "\n";
            g << "G0 Z.1\n";
            g << "G1 Z" << formatGCodeDecimals<4>(-grooveDepth) << " F" << formatGCodeDecimals<3>(ipmOneIPR) << " (.001 IPR)\n";
            g << "G3 X" << formatGCodeDecimals<4>(xFilletRightEnd) << " Y" << formatGCodeDecimals<4>(yFilletRightEnd) << " R" << formatGCodeDecimals<4>(radiusFilletFull) << "\n";
            g << "G0 Z5.\n";
            g << "G0 G91 B" << formatGCodeDecimals<3>(bFilletMoveLeft) << "\n";
            g << "(*****LEFT FILLET*****)\n";
            g << "G0 G90 X" << formatGCodeDecimals<4>(xFilletLeftStart) << " Y" << formatGCodeDecimals<4>(yFilletLeftStart) << "\n";
            g << "G0 Z.1\n";
            g << "G1 Z" << formatGCodeDecimals<4>(-grooveDepth) << " F" << formatGCodeDecimals<3>(ipmOneIPR) << " (.001 IPR)\n";
            g << "G3 X" << formatGCodeDecimals<4>(xFilletLeftEnd) << " Y" << formatGCodeDecimals<4>(yFilletLeftEnd) << " R" << formatGCodeDecimals<4>(radiusFilletFull) << "\n";
            g << "G0 Z5.\n";
            g << "G0 G91 B" << formatGCodeDecimals<3>(bFilletMoveRight) << "\n";

            g << "M99\n";
        }
        g << "%";

        gCodeOutput << g.str();
    }
};

int main() {
    ODHelixGroove job;

    std::cout << "Copyright 2025 <billycarter.business@gmail.com>" << std::endl;
    std::cout << "=============================================================================" << std::endl
              << "                     OD Helical Groove G-code Generator                       " << std::endl
              << "                            Outputs to text file                   " << std::endl
              << "=============================================================================" << std::endl << std::endl;
    std::cout << "Note: You can end any of the dimensions in the suffix 'mm' and it\n";
    std::cout << "      will convert from metric (millimeters) to imperial (inches).\n";
    std::cout << "Note: You can press Escape to return to the previous entry at any point.\n\n";

    job.promptInput();
    job.logInputs();
    job.generateGCode();

    std::ofstream gCodeFile("HelixGroove-OD.txt");
    if (!gCodeFile) {
        std::cerr << "Error: Could not open HelixGroove-OD.txt for writing.\n";
    }
    else {
        gCodeFile << job.gCodeOutput.str();
        std::cout << "\nSuccessfully saved gcode to \"HelixGroove-OD.txt\"";
    }

    std::ofstream logFile("HelixGroove-OD.log.txt");
    if (!logFile) {
        std::cerr << "Error: Could not open HelixGroove-OD.log.txt for writing.\n";
    }
    else {
        logFile << job.inputLog;
        std::cout << "\nSuccessfully saved input log to \"HelixGroove-OD.log.txt\"";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}