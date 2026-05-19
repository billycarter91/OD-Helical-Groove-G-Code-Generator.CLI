#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "gCodeInputAndFormatting.h" // Utility functions for input validation and G-Code formatting

//v1.2 expanded "post processor" to handle mazak integrex i-350h (in addition to original target machine haas vf3)

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
    double xStart;//x coordinate is a reference to the x axis on a cnc mill, which doesn't necessarily pertain to the integrex since technically the z axis is coaxial to the part, but after G68 plane rotation the integrex will use x. Removed mention of x coordinate from user prompts to avoid confusion, but kept variables/math referencing the x axis in this program the same
    double xEnd;
    double cutterDiameter;
    double edgebreakRadius;
    int numFlutes;
    int sfpm;
    double ipt;
    double xClear;

    const double PI = 3.141593; // PI with 6 significant digits for gcode math
    const double EPSILON = 1e-4; // Used to compare floating-point values, accounts for rounding error due to binary representation limits. e.g. use if(abs(currentZ - roughFloorZ) < EPSILON) instead of if(currentZ == roughFloorZ)

    std::string inputLog;
    std::ostringstream gCodeOutputMazakIntegrex;
    std::ostringstream gCodeOutputHaasVF3;

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
                        std::cout << "Must be between 0 and 90 degrees. 90 is parallel to the part'saxis of rotation.\n";
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
                    std::cout << "Enter Feature Start Location (inches, coordinate of the front face of the feature, furthest from the chuck): ";
                    xStart = userInputPosNeg(true);
                    xStart = roundFourDecimal(xStart);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xStart) << "\n";
                    std::cout << std::endl;
                    break;
                }
                case 9: {
                    std::cout << "Enter Feature End Location (inches, coordinate of the back face of the feature, closest to the chuck): ";
                    xEnd = userInputPosNeg(true);
                    xEnd = roundFourDecimal(xEnd);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xEnd) << "\n";
                    if (xEnd <= xStart) {
                        std::cout << "[WARNING] The End point must be closer to the chuck than the Start point.\n";
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
                        std::cout << "\n[NOTE] The normal vane width (remaining material) is " << formatGCodeDecimals<4>(normalVaneWidth) << " wide.\n";
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
                        std::cout << "\n[NOTE]: The normal vane thickness is " << formatGCodeDecimals<4>(normalVaneWidth) << " wide.\n";
                        std::cout << "[NOTE]: The circumferential vane width (aka the vane width when cross-sectioned at " << formatGCodeDecimals<4>(helixAngleDeg) << " deg) is " << formatGCodeDecimals<4>(circumferentialVaneWidth) << "\n";
                        std::cout << "[NOTE]: An edgebreak radius of " << formatGCodeDecimals<4>(edgebreakRadius) << " will remove " << formatGCodeDecimals<4>(circumferentialVaneRemovalFromFilletCut) << " from the circumferential vane width at the exposed face\n";
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
                    if (numFlutes > 30) {
                        std::cout << "I don't think it has that many flutes.\n";
                        continue;
                    }
                    if (numFlutes < 1) {
                        std::cout << "Pick a tool with more cutting edges.\n";
                        continue;
                    }
                    std::cout << std::endl;
                    break;
                }
                case 13: {
                    std::cout << "Enter Cutter Speed (feet/min, or append a single 'm' for meters/min): ";
                    {
                        // Allow a single trailing 'm' for meters/min (will be converted to feet/min).
                        std::string input = getUserInputString(true);
                        double val;
                        std::string err;
                        if (!parseCutterSpeedInput(input, val, err)) {
                            std::cout << err << std::endl;
                            continue;
                        }
                        sfpm = static_cast<int>(round(val));
                        std::cout << "You entered: " << sfpm << " SFPM (" << round(val / 3.280839895) << " meters) which is " << round(sfpm / (PI * (cutterDiameter / 12))) << " RPM\n";
                        if (sfpm > 10000) {
                            std::cout << "A bit fast. Please enter a more reasonable value.\n";
                            continue;
                        }
                        if (sfpm < 20) {
                            std::cout << "A bit slow. Please enter a more reasonable value.\n";
                            continue;
                        }
                        std::cout << std::endl;
                    }
                    break;
                }
                case 14: {
                    std::cout << "Enter Chipload (ipt): ";
                    ipt = userInputPos(true);
                    ipt = roundFourDecimal(ipt);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(ipt) << " IPT (" << formatGCodeDecimals<3>(roundThreeDecimal(ipt * 25.4)) << " mm) which is " << formatGCodeDecimals<3>(roundThreeDecimal(round(sfpm / (PI * (cutterDiameter / 12))) * ipt * numFlutes)) << " IPM (" << formatGCodeDecimals<3>(roundThreeDecimal(round(sfpm / (PI * (cutterDiameter / 12))) * ipt * numFlutes * 25.4)) << " mm)\n";
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
                    std::cout << "Enter Clearance Gap between tool and cutting point (inches): ";//was "Enter Clearance Gap in X", removed "in X" bc clearance gap in X for haas mill, z for mazak integrex.Although after g68 plane rotation, integrex behaves as a mill so 'x' could still be valid, but removed reference to remove confusion
                    xClear = userInputPos(true);
                    xClear = roundFourDecimal(xClear);
                    std::cout << "You entered: " << formatGCodeDecimals<4>(xClear) << "\n";
                    if (xClear < 0.0001) {
                        std::cout << "Enter at least .0001\" clearance.\n";
                        continue;
                    }
                    if (xClear >= (cutterDiameter / 2)) {
                        std::cout << "[NOTE] You do not have to account for half the tool diameter.\n";
                        std::cout << "The gap you enter will be the space between the tool and the cutting point.\n";
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
            "Feature Start Position (inches, face of feature furthest from chuck)",
            "Feature End Position (inches, face of feature nearest chuck)",
            "Cutter Diameter (inches)",
            "Edgebreak Radius (inches)",
            "Number of Flutes",
            "Cutter Speed (sfpm)",
            "Feedrate (ipt)",
            "Clearance Gap (inches)"
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
        std::ostringstream gHaas;
        std::ostringstream gMazak;

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
        // sign handling for N3 y's based on handedness
        if (isRight) {
            yFilletRightStart = -yFilletRightStart;
            yFilletRightEnd = -yFilletRightEnd;
            yFilletLeftStart = -yFilletLeftStart;
            yFilletLeftEnd = -yFilletLeftEnd;
        }
        // correct G2 or G3 direction for fillet based on handedness
        std::string gWhat;
        if (isRight) {
            gWhat = "G3";
        }
        else {
            gWhat = "G2";
        }


        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        ///////////////****MAZAK INTEGREX G-CODE PARSER START****//////////////////// //NOTE HAAS IS ORIGINAL. CONVERSION TO MAZAK CONSISTED OF INVERTING THE ROTARY SIGN, CONVERT TO MM, RAISE Z BY PART RADIUS, NO M97 SUBROUTINE SO USED MACROS
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        int mpm = sfpm / 3.280839895; //feet/min to meters/min
        double mmpt = ipt * 25.4; //inches per tooth to mm per tooth

        //////////////////////////
        // HEADER
        /////////////////////////
        gMazak << "(" << printHandedness << " HAND OD HELIX GROOVES)\n";
        gMazak << "(" << numStarts << " START " << formatGCodeDecimals<2>(helixAngleDeg) << " DEGREES " << formatGCodeDecimals<3>(majorOD * 25.4) << " MAJOR OD " << formatGCodeDecimals<3>((majorOD - (grooveDepth * 2)) * 25.4) << " MINOR OD " << formatGCodeDecimals<3>(normalGrooveWidth * 25.4) << " WIDTH)\n";
        gMazak << "(***POSTED FOR MAZAK INTEGREX i-350H***)\n\n";

        gMazak << "M0 (EDIT VARIABLE #100 TO YOUR TOOL NUMBER)\n";
        gMazak << "#100 = 999 (CHANGE \"999\" TO YOUR TOOL NUMBER FOR THE " << formatGCodeDecimals<2>(cutterDiameter * 25.4) << "MM DIA ENDMILL)\n\n";

        //////////////////////////
        // GROOVE OP
        /////////////////////////
        gMazak << "N100 (CUT GROOVES)\n";
        gMazak << "(" << formatGCodeDecimals<2>(cutterDiameter * 25.4) << "MM DIA ENDMILL " << numFlutes << "FL)\n";
        gMazak << "(" << printHandedness << " HAND OD HELIX GROOVES " << formatGCodeDecimals<3>(normalGrooveWidth * 25.4) << "MM WIDE " << formatGCodeDecimals<3>(grooveDepth * 25.4) << "MM DEEP)\n";
        gMazak << "(CUTTING SPEED : " << mpm << " MPM)\n";
        gMazak << "(CHIP LOAD : " << formatGCodeDecimals<3>(mmpt) << " MMPT)\n";

        gMazak << "G54\n";
        gMazak << "G0 G53 X0. Y0. Z0.\n";
        gMazak << "M200 (TOOL MODE)\n";
        gMazak << "M212\n";
        gMazak << "T#100 M6\n";
        gMazak << "M1\n";
        gMazak << "(INITIALIZE LOOP 1 AND 2 COUNTERS)\n";
        gMazak << "#101 = 1\n";
        gMazak << "#102 = 1\n\n";

        gMazak << "G91 G28 X0. Y0.\n";
        gMazak << "G28 Z0.\n";
        gMazak << "M98 P1000 (TOOLCHANGE)\n";
        gMazak << "G90\n";
        gMazak << "G10.9 X0\n";
        gMazak << "M108\n";
        gMazak << "G53 G0 B0.\n";
        gMazak << "M107\n";
        gMazak << "G19\n";
        gMazak << "M200\n";
        gMazak << "G0 C0\n";
        gMazak << "M212\n";
        gMazak << "G0 C0. (INITIAL C COORDINATE)\n";
        gMazak << "M210\n";
        gMazak << "G69\n";
        gMazak << "G0 B90.\n";
        gMazak << "G68 X0. Y0. Z0. I0. J1. K0. R90.\n";
        gMazak << "G17\n";
        gMazak << "G0 X-200. Y0. Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 50) << " (50MM ABOVE OD)\n";
        gMazak << "G97 G95 S" << rpm << " M3\n";
        gMazak << "M8\n\n";

        gMazak << "G0 X" << formatGCodeDecimals<3>((xStart - toolClear) * 25.4) << " (" << formatGCodeDecimals<3>(xClear * 25.4) << "MM CLEAR)\n";
        gMazak << "M212\n\n";

        gMazak << "WHILE [#101 LE " << numStarts << "] DO 1\n";
        gMazak << "  G91 C" << formatGCodeDecimals<3>(bIndex) << "\n";
        gMazak << "  #102 = 1\n";
        gMazak << "  WHILE [#102 LE " << numDepthPasses << "] DO 2\n";
        gMazak << "    G0 G90 X" << formatGCodeDecimals<3>((xStart - toolClear) * 25.4) << " Y0.\n";
        gMazak << "    G0 G91 Z-48.\n";
        gMazak << "    G1 Z-2. F" << formatGCodeDecimals<3>(ipt * numFlutes * 25.4) << " (MMPR)\n";
        gMazak << "    Z" << formatGCodeDecimals<3>(-depthPerPass * 25.4) << " (INCREMENTAL DEPTH PER PASS)\n\n";

        gMazak << "    (BRANCH CUT STRATEGY START)\n";

        // cut strategy selection
        if (fabs(normalGrooveWidth - cutterDiameter) < EPSILON) {
            gMazak << "    (*****SINGLE PATH WITH ON-SIZE TOOL*****)\n";
            gMazak << "      G07.1 C" << formatGCodeDecimals<3>(majorOD * 25.4) << "\n";
            gMazak << "    G1 X" << formatGCodeDecimals<3>(forwardX * 25.4) << " C" << formatGCodeDecimals<3>(-forwardB) << "\n";
            gMazak << "      G07.1 C0\n";
            gMazak << "    G0 Z50.\n";
            gMazak << "    G0 X" << formatGCodeDecimals<3>(returnX * 25.4) << " C" << formatGCodeDecimals<3>(-returnB) << "\n";
        }
        else {
            gMazak << "    (*****DUAL PATH WITH UNDERSIZED TOOL*****)\n";
            gMazak << "    (FORWARD PASS ON ONE WALL)\n";
            gMazak << "    G1 Y" << formatGCodeDecimals<3>(forwardYOffset * 25.4) << "\n";
            gMazak << "      G07.1 C" << formatGCodeDecimals<3>(majorOD * 25.4) << "\n";
            gMazak << "    G1 X" << formatGCodeDecimals<3>(forwardX * 25.4) << " C" << formatGCodeDecimals<3>(-forwardB) << "\n";
            gMazak << "      G07.1 C0\n";
            gMazak << "    (REVERSE PASS ON OPPOSITE WALL)\n";
            gMazak << "    G1 Y" << formatGCodeDecimals<3>(returnYOffset * 2.0 * 25.4) << "\n";
            gMazak << "      G07.1 C" << formatGCodeDecimals<3>(majorOD * 25.4) << "\n";
            gMazak << "    G1 X" << formatGCodeDecimals<3>(returnX * 25.4) << " C" << formatGCodeDecimals<3>(-returnB) << "\n";
            gMazak << "      G07.1 C0\n";
            gMazak << "    G0 Z50.\n";
            gMazak << "    G0 G90 Y0.\n";
        }

        gMazak << "    (BRANCH CUT STRATEGY END)\n\n";
        gMazak << "    #102 = #102 + 1\n";
        gMazak << "  END 2\n\n";

        gMazak << "  G0 G90 Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 50) << " (50MM ABOVE OD)\n";
        gMazak << "  #101 = #101 + 1\n";
        gMazak << "END 1\n";
        gMazak << "G0 X-200. M9\n\n";

        gMazak << "G49\n";
        gMazak << "G69\n";
        gMazak << "G53 X0.\n";
        gMazak << "G53 Z0. Y0.\n";
        gMazak << "G53 Y-90.\n";
        gMazak << "M0\n\n";

        //////////////////////////
        // FILLET OP
        /////////////////////////
        if (edgebreakRadius > EPSILON) {
            gMazak << "N200 (CUT EDGEBREAK FILLETS)\n";
            gMazak << "(" << formatGCodeDecimals<2>(cutterDiameter * 25.4) << "MM DIA ENDMILL " << numFlutes << "FL)\n";
            gMazak << "(CUTTING SPEED : " << mpm << " MPM)\n";

            gMazak << "G54\n";
            gMazak << "G0 G53 X0. Y0. Z0.\n";
            gMazak << "M200 (TOOL MODE)\n";
            gMazak << "M212\n";
            gMazak << "T#100 M6\n";
            gMazak << "M1\n";
            gMazak << "(INITIALIZE LOOP 3 COUNTER)\n";
            gMazak << "#103 = 1\n\n";

            gMazak << "G91 G28 X0. Y0.\n";
            gMazak << "G28 Z0.\n";
            gMazak << "M98 P1000 (TOOLCHANGE)\n";
            gMazak << "G90\n";
            gMazak << "G10.9 X0\n";
            gMazak << "M108\n";
            gMazak << "G53 G0 B0.\n";
            gMazak << "M107\n";
            gMazak << "G19\n";
            gMazak << "M200\n";
            gMazak << "G0 C0\n";
            gMazak << "M212\n";
            gMazak << "G0 C" << formatGCodeDecimals<3>(-bFilletInitialRight) << " (INITIAL C COORDINATE MAPPED TO LEFT END FILLET START)\n";
            gMazak << "M210\n";
            gMazak << "G69\n";
            gMazak << "G0 B90.\n";
            gMazak << "G68 X0. Y0. Z0. I0. J1. K0. R90.\n";
            gMazak << "G17\n";
            gMazak << "X-200. Y0. Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 50) << " (50MM ABOVE OD)\n";
            gMazak << "G97 G95 S" << rpm << " M3\n";
            gMazak << "M8\n\n";

            gMazak << "G0 X" << formatGCodeDecimals<3>((xStart - toolClear) * 25.4) << " (" << formatGCodeDecimals<3>(xClear * 25.4) << "MM CLEAR)\n";
            gMazak << "M212\n\n";

            gMazak << "WHILE [#103 LE " << numStarts << "] DO 3 \n";
            gMazak << "  G91 C" << formatGCodeDecimals<3>(bIndex) << "\n\n";

            gMazak << "  (OD HELIX GROOVE - R" << formatGCodeDecimals<3>(edgebreakRadius * 25.4) << "MM EDGEBREAK)\n";
            gMazak << "  (*****LEFT FILLET*****)\n";
            gMazak << "  G0 G90 X" << formatGCodeDecimals<3>(xFilletRightStart * 25.4) << " Y" << formatGCodeDecimals<3>(yFilletRightStart * 25.4) << "\n";
            gMazak << "  G0 Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 2) << "\n";
            gMazak << "  G1 Z" << formatGCodeDecimals<3>(((majorOD / 2) - grooveDepth) * 25.4) << " F.025 (.025 MMPR)\n";
            gMazak << "  " << gWhat << " X" << formatGCodeDecimals<3>(xFilletRightEnd * 25.4) << " Y" << formatGCodeDecimals<3>(yFilletRightEnd * 25.4) << " R" << formatGCodeDecimals<3>(radiusFilletFull * 25.4) << "\n";
            gMazak << "  G0 Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 50) << "\n";
            gMazak << "  G0 G91 C" << formatGCodeDecimals<3>(-bFilletMoveLeft) << "\n";
            gMazak << "  (*****RIGHT FILLET*****)\n";
            gMazak << "  G0 G90 X" << formatGCodeDecimals<3>(xFilletLeftStart * 25.4) << " Y" << formatGCodeDecimals<3>(yFilletLeftStart * 25.4) << "\n";
            gMazak << "  G0 Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 2) << "\n";
            gMazak << "  G1 Z" << formatGCodeDecimals<3>(((majorOD / 2) - grooveDepth) * 25.4) << " F.025 (.025 MMPR)\n";
            gMazak << "  " << gWhat << " X" << formatGCodeDecimals<3>(xFilletLeftEnd * 25.4) << " Y" << formatGCodeDecimals<3>(yFilletLeftEnd * 25.4) << " R" << formatGCodeDecimals<3>(radiusFilletFull * 25.4) << "\n";
            gMazak << "  G0 Z" << formatGCodeDecimals<3>(((majorOD / 2) * 25.4) + 50) << "\n";
            gMazak << "  G0 G91 C" << formatGCodeDecimals<3>(-bFilletMoveRight) << "\n\n";

            gMazak << "  #103 = #103 + 1\n";
            gMazak << "END 3\n";
            gMazak << "X-200. M9\n\n";

            gMazak << "G49\n";
            gMazak << "G69\n";
            gMazak << "G53 X0.\n";
            gMazak << "G53 Z0. Y0.\n";
            gMazak << "G53 Y-90.\n";
            gMazak << "M30";
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        ////////////////****MAZAK INTEGREX G-CODE PARSER END****/////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        //////////////////************************************///////////////////////

        //////////////////************************************///////////////////////

        //////////////////************************************///////////////////////

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        //////////////////****HAAS VF3 G-CODE PARSER START****///////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        //////////////////////////
        // HEADER
        /////////////////////////
        gHaas << "%\n";
        gHaas << "O1 (DwgNo ISSUE -)\n";
        gHaas << "(ProgrammerName Mo-Da-Year)\n";
        gHaas << "(Material)\n";
        gHaas << "(" << printHandedness << " HAND OD HELIX GROOVES)\n";
        gHaas << "(***POSTED FOR HAAS VF3***)\n";
        gHaas << "(PROGRAMMED FOR A CHUCK ON THE RIGHT END OF MACHINE TABLE)\n";
        gHaas << "(VERIFY B AXIS ISN'T REVERSED BY MACHINE PARAMETERS)\n\n";

        gHaas << "(OP 1)\n\n";

        gHaas << "(BUTTON FIXTURE)\n";
        gHaas << "(0.0000 OD OUT)\n";
        gHaas << "(SET Z ON " << formatGCodeDecimals<4>(majorOD) << " OD)\n";
        gHaas << "(SET X ON LEFT FACE)\n\n";

        gHaas << "M0(EDIT HAAS SETTING 79 5TH AXIS DIAMETER TO THE)\n"; // haas complains long comment - added linebreak
        gHaas << "(MAJOR OD OF THE VANES " << formatGCodeDecimals<4>(majorOD) << " FOR FEEDRATE CALCULATION)\n\n";

        //////////////////////////
        // GROOVE OP
        /////////////////////////
        gHaas << "T5 M06 (" << formatGCodeDecimals<4>(cutterDiameter) << " DIA ENDMILL " << numFlutes << "FL)\n";
        gHaas << "(" << printHandedness << " HAND OD HELIX GROOVES " << formatGCodeDecimals<4>(normalGrooveWidth) << " WIDE " << formatGCodeDecimals<4>(grooveDepth) << " DEEP)\n";
        gHaas << "(CUTTING SPEED : " << sfpm << " SFPM)\n";
        gHaas << "(CHIP LOAD : " << formatGCodeDecimals<4>(ipt) << " IPT)\n";
        gHaas << "(REMAINING STOCK : XY:0. | Z:0.)\n";
        gHaas << "G0 G91 G28 Z0.\n";
        gHaas << "G0 G91 G28 Y0.\n";
        gHaas << "G0 G17 G90 G56 A-90. B0.\n";
        gHaas << "G0 X-8. Y0. S" << rpm << " M03\n";
        gHaas << "G0 G43 H05 Z5. M8\n";
        gHaas << "G0 X" << formatGCodeDecimals<4>(xStart - toolClear) << " (" << formatGCodeDecimals<4>(xClear) << " CLEAR)\n";
        gHaas << "G91 M97 P1 L" << numStarts << " B" << formatGCodeDecimals<3>(bIndex) << "\n";
        gHaas << "G0 G90 Z5.\n";
        gHaas << "X-8. M9\n";
        gHaas << "G0 G91 G28 Z0.\n";
        gHaas << "G0 G91 G28 Y0.\n";
        gHaas << "M0\n\n";

        //////////////////////////
        // FILLET OP
        /////////////////////////
        if (edgebreakRadius > EPSILON) {
            gHaas << "T5 M06 (" << formatGCodeDecimals<4>(cutterDiameter) << " DIA ENDMILL " << numFlutes << "FL)\n";
            gHaas << "(" << printHandedness << " HAND OD HELIX GROOVES - R" << formatGCodeDecimals<4>(edgebreakRadius) << " EDGEBREAK)\n";
            gHaas << "(CUTTING SPEED : " << sfpm << " SFPM)\n";
            gHaas << "(CHIP LOAD : .001 IPR)\n";
            gHaas << "(REMAINING STOCK : XY:0. | Z:0.)\n";
            gHaas << "G0 G91 G28 Z0.\n";
            gHaas << "G0 G91 G28 Y0.\n";
            gHaas << "G0 G17 G90 G56 A-90. B" << formatGCodeDecimals<3>(bFilletInitialRight) << " (INITIAL B COORDINATE MAPPED TO RIGHT END FILLET START)\n";
            gHaas << "G0 X-8. Y0. S" << rpm << " M03\n";
            gHaas << "G0 G43 H05 Z5. M8\n";
            gHaas << "G0 X" << formatGCodeDecimals<4>(xStart - toolClear) << " (" << formatGCodeDecimals<4>(xClear) << " CLEAR)\n";
            gHaas << "G91 M97 P3 L" << numStarts << " B" << formatGCodeDecimals<3>(bIndex) << "\n";
            gHaas << "G0 G90 Z5.\n";
            gHaas << "X-8. M9\n";
            gHaas << "G0 G91 G28 Z0.\n";
            gHaas << "G0 G91 G28 Y0.\n";
            gHaas << "M30\n\n";
        }

        //////////////////////////
        // N1 DEPTH PASSES SUBROUTINE
        /////////////////////////
        gHaas << "N1 (OD HELIX GROOVE - DEPTH PASSES SUBROUTINE)\n";
        gHaas << "G91 M97 P2 L" << numDepthPasses << " Z" << formatGCodeDecimals<4>(-depthPerPass) << "\n";
        gHaas << "G0 G90 Z5.\n";
        gHaas << "M99\n\n";

        //////////////////////////
        // N2 GROOVE SUBROUTINE - FIX Y, MOVE X/ROTARY
        /////////////////////////
        gHaas << "N2 (OD HELIX GROOVE - CUT GROOVES SUBROUTINE)\n";
        gHaas << "G0 G90 X" << formatGCodeDecimals<4>(xStart - toolClear) << " Y0.\n";
        gHaas << "G0 G91 Z" << formatGCodeDecimals<4>(-5.0 + .1) << "\n";
        gHaas << "G1 Z" << formatGCodeDecimals<4>(-depthPerPass - .1) << " F" << formatGCodeDecimals<3>(ipm) << " (" << formatGCodeDecimals<4>(ipt) << " IPT)\n";

        // cut strategy selection
        if (fabs(normalGrooveWidth - cutterDiameter) < EPSILON) {
            gHaas << "(*****SINGLE PATH WITH ON-SIZE TOOL*****)\n";
            gHaas << "G1 X" << formatGCodeDecimals<4>(forwardX) << " B" << formatGCodeDecimals<4>(forwardB) << "\n";
            gHaas << "G0 Z5.\n";
            gHaas << "G0 X" << formatGCodeDecimals<4>(returnX) << " B" << formatGCodeDecimals<4>(returnB) << "\n";
        }
        else {
            gHaas << "(*****DUAL PATH WITH UNDERSIZED TOOL*****)\n";
            gHaas << "(FORWARD PASS ON ONE WALL)\n";
            gHaas << "G1 Y" << formatGCodeDecimals<4>(forwardYOffset) << "\n";
            gHaas << "G1 X" << formatGCodeDecimals<4>(forwardX) << " B" << formatGCodeDecimals<4>(forwardB) << "\n";
            gHaas << "(REVERSE PASS ON OPPOSITE WALL)\n";
            gHaas << "G1 Y" << formatGCodeDecimals<4>(returnYOffset * 2.0) << "\n";
            gHaas << "G1 X" << formatGCodeDecimals<4>(returnX) << " B" << formatGCodeDecimals<4>(returnB) << "\n";
            gHaas << "G0 Z5.\n";
            gHaas << "G0 G90 Y0.\n";
        }

        gHaas << "M99\n";

        //////////////////////////
        // N3 FILLET SUBROUTINE - FIX ROTARY, MOVE X/Y
        /////////////////////////
        if (edgebreakRadius > EPSILON) {
            gHaas << "\nN3 (OD HELIX GROOVE - R" << formatGCodeDecimals<4>(edgebreakRadius) << " EDGEBREAK SUBROUTINE)\n";
            gHaas << "(*****RIGHT FILLET*****)\n";
            gHaas << "G0 G90 X" << formatGCodeDecimals<4>(xFilletRightStart) << " Y" << formatGCodeDecimals<4>(yFilletRightStart) << "\n";
            gHaas << "G0 Z.1\n";
            gHaas << "G1 Z" << formatGCodeDecimals<4>(-grooveDepth) << " F" << formatGCodeDecimals<3>(ipmOneIPR) << " (.001 IPR)\n";
            gHaas << gWhat << " X" << formatGCodeDecimals<4>(xFilletRightEnd) << " Y" << formatGCodeDecimals<4>(yFilletRightEnd) << " R" << formatGCodeDecimals<4>(radiusFilletFull) << "\n";
            gHaas << "G0 Z5.\n";
            gHaas << "G0 G91 B" << formatGCodeDecimals<3>(bFilletMoveLeft) << "\n";
            gHaas << "(*****LEFT FILLET*****)\n";
            gHaas << "G0 G90 X" << formatGCodeDecimals<4>(xFilletLeftStart) << " Y" << formatGCodeDecimals<4>(yFilletLeftStart) << "\n";
            gHaas << "G0 Z.1\n";
            gHaas << "G1 Z" << formatGCodeDecimals<4>(-grooveDepth) << " F" << formatGCodeDecimals<3>(ipmOneIPR) << " (.001 IPR)\n";
            gHaas << gWhat << " X" << formatGCodeDecimals<4>(xFilletLeftEnd) << " Y" << formatGCodeDecimals<4>(yFilletLeftEnd) << " R" << formatGCodeDecimals<4>(radiusFilletFull) << "\n";
            gHaas << "G0 Z5.\n";
            gHaas << "G0 G91 B" << formatGCodeDecimals<3>(bFilletMoveRight) << "\n";

            gHaas << "M99\n";
        }
        gHaas << "%";


        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        ///////////////////****HAAS VF3 G-CODE PARSER END/****///////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////


        gCodeOutputMazakIntegrex << gMazak.str();
        gCodeOutputHaasVF3 << gHaas.str();
    }
};

int main() {
    ODHelixGroove job;

    std::cout << "Copyright 2025 <billycarter.business@gmail.com>" << std::endl;
    std::cout << "=============================================================================" << std::endl
        << "                  OD Helical Groove G-code Generator v1.2.0                  " << std::endl
        << "            Creates .eia file for Mazak Integrex i-350H  (Metric)            " << std::endl
        << "                  Creates .nc file for Haas VF3  (Imperial)                  " << std::endl
        << "=============================================================================" << std::endl << std::endl;
    std::cout << "Note: You can end any of the dimensions with the suffix 'mm' and it\n";
    std::cout << "      will convert from metric (millimeters) to imperial (inches).\n";
    std::cout << "Note: You can press Escape to return to the previous entry at any point.\n\n";

    job.promptInput();
    job.logInputs();
    job.generateGCode();

    std::ofstream gCodeFileMazak("HelixGroove-OD.Mazak.Integrex.eia");
    if (!gCodeFileMazak) {
        std::cerr << "Error: Could not open HelixGroove-OD.Mazak.Integrex.eia for writing.\n";
    }
    else {
        gCodeFileMazak << job.gCodeOutputMazakIntegrex.str();
        std::cout << "\nSuccessfully saved gcode to \"HelixGroove-OD.Mazak.Integrex.eia\"";
    }

    std::ofstream gCodeFileHaas("HelixGroove-OD.Haas.VF3.nc");
    if (!gCodeFileHaas) {
        std::cerr << "Error: Could not open HelixGroove-OD.Haas.VF3.nc for writing.\n";
    }
    else {
        gCodeFileHaas << job.gCodeOutputHaasVF3.str();
        std::cout << "\nSuccessfully saved gcode to \"HelixGroove-OD.Haas.VF3.nc\"";
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