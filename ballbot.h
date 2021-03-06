/* Ballbot.h - Header file for ballbot program
 *
 * (C) Brian Chen 2015
 */

#define SPI_CLOCK 8000000

#define SCK_PIN  14
#define SS_PIN   10 
#define INT_PIN  3
#define LED      13
#define LED2     28

#define BLINK_OK_INTERVAL 500
#define BLINK_SENSOR_ERR_INTERVAL 200
#define BLINK_STOPPED_INTERVAL 75
#define BLINK_CALIB_MOTOR_INTERVAL 2000

#define PWM_FREQ  16000

#define MT_A_DIR  4
#define MT_A_PWM  5
#define MT_A_BRK  16
#define MT_A_FLIP false
#define MT_A_CS   A10

#define MT_B_DIR  9
#define MT_B_PWM  23
#define MT_B_BRK  7
#define MT_B_FLIP false
#define MT_B_CS   A14

#define MT_C_DIR  8
#define MT_C_PWM  6
#define MT_C_BRK  22
#define MT_C_FLIP false
#define MT_C_CS   A11

#define INT_UPDATE_INTERVAL 18

#define ENCXA       20
#define ENCXB       15
#define ENCYA       21
#define ENCYB       17
#define ENCZA       3
#define ENCZB       2

#define WHEEL_R    24
#define CHASSIS_R  88
#define CPR        64  // though effectively 256
#define GEAR_RATIO 16
#define MAX_RPM    12400
#define CPRAD      651.898644   // 4x counts per radian

String pgm_version = "1.0";

int blinkInterval = BLINK_OK_INTERVAL;

MPU9250 mpu(SPI_CLOCK, SS_PIN, BITS_DLPF_CFG_10HZ, BITS_DLPF_CFG_10HZ);

int16_t mag_max[3];
int16_t mag_min[3];

float roll, pitch, yaw;
float roll_offset, pitch_offset;

float mag_scalar[3], mag_bias[3];


/* since there are 4 phases a quadrature encoder goes through, there are 16 combinations, many of which
   are invalid (and hence = 0) */
const int32_t enc_TAB[]= { 0, 1, -1, 0,
                          -1, 0,  0, 1,
                           1, 0,  0,-1,
                           0,-1,  1, 0 };

volatile uint32_t encState = 0, lEncState = 0;
volatile uint32_t encIndex;

volatile uint32_t channelCount = 0, intUpdateCount = 0;

volatile int32_t rtA = 0, rtB = 0, rtC = 0, rtD = 0;    // Realtime values
volatile int32_t prA = 0, prB = 0, prC = 0, prD = 0;    // Previous values

float vA = 0, vB = 0, vC = 0, vD = 0;    // wheel velocity (m/s)
float wA = 0, wB = 0, wC = 0, wD = 0;   // angular velocity (rad/s)
float tA = 0, tB = 0, tC = 0, tD = 0;   // wheel angle

elapsedMicros encUpdateDt;

float vA_targ, vB_targ, vC_targ, vD_targ; // target angular velocity

PMOTOR motorA(MT_A_PWM, MT_A_DIR, MT_A_BRK, MT_A_FLIP, MT_A_CS);
PMOTOR motorB(MT_B_PWM, MT_B_DIR, MT_B_BRK, MT_B_FLIP, MT_B_CS);
PMOTOR motorC(MT_C_PWM, MT_C_DIR, MT_C_BRK, MT_C_FLIP, MT_C_CS);

int32_t pA, pB, pC;
float pA_f, pB_f, pC_f;
float pidOutA, pidOutB, pidOutC;
uint8_t motorMinPwr = 0;
float motorPwrScalar = 0.8;

bool motorPidControl = false;

float mtrKp = 6, mtrKi, mtrKd = 0.04;
PID pidMtrA(&vA, &pidOutA, &vA_targ, mtrKp, mtrKi, mtrKd, REVERSE);
PID pidMtrB(&vB, &pidOutB, &vB_targ, mtrKp, mtrKi, mtrKd, REVERSE);
PID pidMtrC(&vC, &pidOutC, &vC_targ, mtrKp, mtrKi, mtrKd, REVERSE);

elapsedMicros pidMtrdt;

// omnidrive omni(&pA, &pB, &pC);
omnidrive omni(&vA_targ, &vB_targ, &vC_targ);

elapsedMicros readIMUdt;
elapsedMicros readMagDt;

float v_x, v_y;
float pos_x, pos_y;
float theta;

BalanceController bController(&v_x, &v_y, &pos_x, &pos_y, &theta, &roll, &pitch);
LocationCalculator lCalculator(CHASSIS_R, 180, 60, 300, &vA, &vB, &vC);

bool btDebug = true;
bool serDebug = true;

bool calibMotorMode = false;
bool calibMotorModeDir = true;
elapsedMicros calibMotorModeDt;
int calibMotorModeAmplitude = 10;

uint32_t loopCount = 0;
uint32_t pidUpdateCount = 0;

unsigned long now;


static void storeBalanceTunings();
static void readBalanceTunings();
static void storePosTunings();
static void readPosTunings();
static void storeState();
static void readState();
static void storeIMUOffset();
static void readIMUOffset();
void zeroImu();
static void storeMotorMinPwr();
static void readMotorMinPwr();
void calibMagRoutine();
void calcMagCalib();
static void storeMagCalib();
static void readMagCalib();
void mpuRead();
void fusionUpdate();
void fusionGetEuler();
inline void encoderPoll();
inline void encoderGetVelocity();
void calibMotorModeRoutine();

// controller for serial interface
class SerialController : public Print{
public:
    SerialController(Stream& _port): port(_port) {}

    void sendNames(){
        print("$");
        print("now");                   print('\t');
        print("roll");                  print('\t');
        print("pitch");                 print('\t');
        print("e_theta_x*kp");          print('\t');
        print("e_theta_y*kp");          print('\t');    
        print("int_theta_x");           print('\t');
        print("int_theta_y");           print('\t');
        print("d_theta_x*kd_theta");    print('\t');
        print("d_theta_y*kd_theta");    print('\t');

        print("e_v_x*kp_v"); print('\t');
        print("e_v_y*kp_v"); print('\t');    
        print("int_v_x");    print('\t');
        print("int_v_y");    print('\t');
        print("d_v_x*kd_v"); print('\t');
        print("d_v_y*kd_v"); print('\t');

        print("e_pos_x*kp_pos"); print('\t');
        print("e_pos_y*kp_pos"); print('\t');    
        print("int_pos_x");      print('\t');
        print("int_pos_y");      print('\t');
        print("d_pos_x*kd_pos"); print('\t');
        print("d_pos_y*kd_pos"); print('\t');
        
        print("v_x");      print('\t');
        print("v_y");      print('\t');
        print("pA");       print('\t');
        print("pB");       print('\t');
        print("pC");       print('\t');
        print("pos_x");    print('\t');
        println("pos_y");
    }
    void debug(){
        print(now);                                             print('\t');
        print(roll);                                            print('\t');
        print(pitch);                                           print('\t');
        print(bController.e_theta_x * bController.kp_theta);    print('\t');
        print(bController.e_theta_y * bController.kp_theta);    print('\t');    
        print(bController.int_theta_x);                         print('\t');
        print(bController.int_theta_y);                         print('\t');
        print(bController.d_theta_x * bController.kd_theta);    print('\t');
        print(bController.d_theta_y * bController.kd_theta);    print('\t');

        print(bController.e_v_x * bController.kp_v); print('\t');
        print(bController.e_v_y * bController.kp_v); print('\t');    
        print(bController.int_v_x);                  print('\t');
        print(bController.int_v_y);                  print('\t');
        print(bController.d_v_x * bController.kd_v); print('\t');
        print(bController.d_v_y * bController.kd_v); print('\t');
        
        print(bController.e_pos_x * bController.kp_pos); print('\t');
        print(bController.e_pos_y * bController.kp_pos); print('\t');    
        print(bController.int_pos_x);                    print('\t');
        print(bController.int_pos_y);                    print('\t');
        print(bController.d_pos_x * bController.kd_pos); print('\t');
        print(bController.d_pos_y * bController.kd_pos); print('\t');
        
        print(v_x);      print('\t');
        print(v_y);      print('\t');
        print(pA);       print('\t');
        print(pB);       print('\t');
        print(pC);       print('\t');
        print(pos_x);    print('\t');
        println(pos_y);
    }

    // magnetometer calibration routine
    void calibMag(){
    	bcBalanceEnabled = bController.balanceEnabled();
        bcPosCorEnabled = bController.posCorEnabled();

        bController.disable();

        port.println("$Starting magnetometer calibration. Send any string to finish calibration.");                    
        calibMagRoutine();
        port.println("$Finished calibration");
        storeMagCalib();

        if (bcBalanceEnabled) bController.enableBalance();
        if (bcPosCorEnabled)  bController.enablePosCorrection();
    }

    void info(){
    	port.println("$Status: ");
        port.print("$\t");
        port.print("balanceEnabled = ");
        port.print(bController.balanceEnabled()); port.print(", ");
        port.print("posCorEnabled = ");
        port.print(bController.posCorEnabled());  port.print(", ");
        port.print("calibMotorMode = ");
        port.print(calibMotorMode);               port.print(", ");
        port.print("min motor power = ");         port.print(", ");
        port.print(motorMinPwr);                  port.print(", ");
        port.print("blinkInterval = ");
        port.print(blinkInterval);
        port.print(" (");
        switch(blinkInterval){
            case BLINK_OK_INTERVAL:             port.print("BLINK_OK_INTERVAL");          break;
            case BLINK_STOPPED_INTERVAL:        port.print("BLINK_STOPPED_INTERVAL");     break;                
            case BLINK_CALIB_MOTOR_INTERVAL:    port.print("BLINK_CALIB_MOTOR_INTERVAL"); break;                        
            case BLINK_SENSOR_ERR_INTERVAL:     port.print("BLINK_SENSOR_ERR_INTERVAL");  break;                
            default:                            port.print("UNKNOWN blinkInterval");      break;                        
        }
        port.println(")");
        port.println("$Tunings: ");
        port.print("$\t");
        port.print(bController.kp_theta, 8);     port.print(", ");
        port.print(bController.ki_theta, 8);     port.print(", ");
        port.print(bController.kd_theta, 8);     port.print(", ");
        port.print(bController.kp_v, 8);    port.print(", ");
        port.print(bController.ki_v, 8);    port.print(", ");
        port.print(bController.kd_v, 8);    port.print(", ");
        port.print(bController.kp_pos, 8);       port.print(", ");
        port.print(bController.ki_pos, 8);       port.print(", ");
        port.println(bController.kd_pos, 8);
        port.println("$Magnetometer:");
        port.print("$\t");
        port.print("mag_scalar:\t{");
        port.print(mag_scalar[0]); port.print(", ");
        port.print(mag_scalar[1]); port.print(", ");
        port.print(mag_scalar[2]);
        port.print("}\tmag_bias:\t{");
        port.print(mag_bias[0]); port.print(", ");
        port.print(mag_bias[1]); port.print(", ");
        port.print(mag_bias[2]);
        port.println("}");
        port.println("$Stored calibration");
    }

    void disableDebug(){
    	btDebug = false;
        serDebug = false;
    }

    void enableDebug(){
    	btDebug = true;
        serDebug = true;
    }

    // Reads number array from string with a comma delimiter.
    // If the number of elements is unequal to len, an error
    // message will be sent through the port.
    bool readData(char buff[], float my_array[], int len){
        int i = 0;
        char *tok = strtok(buff, ",");
        while (tok != 0 && i < len){
            my_array[i] = atof(tok);
            tok = strtok(0, ",");
            // port.println(my_array[i]);
            i++;
        }
        if (tok != 0 || i != len){
            port.println("$Incorrect number of inputs:");
            port.print("$\t");
            port.print(len);
            port.println(" comma separated values required");
            return false;
        }
        return true;
    }

    // essentially a large switch that performs commands based
    // on port input
    void update(){
        if (ready){
            ready = false;

            char command = buffer[0];
            buffer[0] = ',';

            switch(command){
                case 'f':
                    {
                    if (buffer[1] == 'e')
                        bController.enablePosCorFlip();
                    else if (buffer[1] == 'd')
                        bController.disablePosCorFlip();
                    }
                    break;
                case 'c':
                    calibMag();
                    break;
                case 'i':
                    info();
                    break;
                case 't':    // set PID parameters
                    {
                    bcBalanceEnabled = bController.balanceEnabled();
                    bcPosCorEnabled = bController.posCorEnabled();
                    bController.disable();

                    float my_array[9];

                    if (readData(buffer, my_array, 9)){
                        bController.setTunings(my_array[0], my_array[1], my_array[2],
                                               my_array[3], my_array[4], my_array[5],
                                               my_array[6], my_array[7], my_array[8]);
                        port.println("$Set PID parameters: ");      port.print('\t');                        
                        port.print(my_array[0], 8);                port.print('\t');
                        port.print(my_array[1], 8);                port.print('\t');
                        port.print(my_array[2], 8);                port.print('\t');
                        port.print(my_array[3], 8);                port.print('\t');
                        port.print(my_array[4], 8);                port.print('\t');
                        port.print(my_array[5], 8);                port.print('\t');
                        port.print(my_array[6], 8);                port.print('\t');
                        port.print(my_array[7], 8);                port.print('\t');
                        port.println(my_array[8], 8);
                    }

                    //delay(1000);
                    if (bcBalanceEnabled) bController.enableBalance();
                    if (bcPosCorEnabled)  bController.enablePosCorrection();
                    }
                    break;
                case 'e':
                    {
                    if (buffer[1] == 'b'){
                        port.println("$Enabled balancing");
                        bController.enableBalance();
                    }
                    else if (buffer[1] == 'p'){
                        port.println("$Enabled pos correction");
                        bController.enablePosCorrection();
                    }
                    else{
                        port.println("$Enabled bController");
                        bController.enable();
                    }
                    }
                    break;
                case 'd':
                    {
                    if (buffer[1] == 'b'){
                        port.println("$Disabled balancing");
                        bController.disableBalance();
                    }
                    else if (buffer[1] == 'p'){
                        port.println("$Disabled pos correction");
                        bController.disablePosCorrection();
                    }
                    else{
                        port.println("$Disabled bController");
                        bController.disable();
                    }
                    }
                    break;
                case '?':
                	sendNames();
                	break;
                case ']':
					disableDebug();                    
                    break;
                case '[':
                	enableDebug();                    
                    break;
                case '+':
                    {
                    float my_array[1];
                    if (readData(buffer, my_array, 1)){
                        if (my_array[0] <= 0){
                            port.println("$Amplitude cannot be negative or zero");
                        }
                        else if (my_array[0] <= 20){
                            port.println("$Minimum amplitude of 20 required");
                        }
                        else if (my_array[0] > 255){
                            port.println("$Maximum amplitude of 255");
                        }
                        else{
                            port.print("$Amplitude set at ");
                            calibMotorModeAmplitude = (int)my_array[0];
                            port.println("$Enabled calib motor mode.");
                            port.println("$Disabled balancing (if not already)");
                            port.println("$---");

                            calibMotorMode = true;
                            calibMotorModeDt = 0;
                            pA = 0; pB = 0; pC = 0;

                            bcBalanceEnabled = bController.balanceEnabled();
                            bcPosCorEnabled = bController.posCorEnabled();

                            bController.disable();
                            blinkInterval = BLINK_CALIB_MOTOR_INTERVAL;
                        }                        
                    }
                    delay(1000);
                    }                    
                    break;
                case '-':
                    port.println("$Disabled calib motor mode.");
                    delay(1000); // so you can see the text
                    calibMotorMode = false;
                    pA = 0; pB = 0; pC = 0;
                    blinkInterval = BLINK_OK_INTERVAL;
                    if (bcBalanceEnabled) bController.enableBalance();
                    if (bcPosCorEnabled)  bController.enablePosCorrection();
                    break;
                case '_':
                    {
                    bController.disable();
                    float my_array[1];

                    if (readData(buffer, my_array, 1)){
                        port.print("$Set motorMinPwr from ");
                        port.print(motorMinPwr);
                        port.print(" to ");
                        motorMinPwr = (uint8_t)my_array[0];
                        port.println(motorMinPwr);
                    }

                    // delay(1000);
                    bController.enable();
                    }
                    break;
                case 'z':
                    {
                    bcBalanceEnabled = bController.balanceEnabled();
                    bcPosCorEnabled = bController.posCorEnabled();

                    if (buffer[1] == 'i'){
                        zeroImu();
                        port.print("$Zeroed at r = ");
                        port.print(roll_offset);
                        port.print(" p = ");
                        port.println(pitch_offset);
                    }
                    else if (buffer[1] == 'p'){
                        pos_x = 0;
                        pos_y = 0;
                        lCalculator.zero();
                        port.println("$Zeroed position");
                    }

                    if (bcBalanceEnabled) bController.enableBalance();
                    if (bcPosCorEnabled)  bController.enablePosCorrection();
                    }
                    break;
                case 's':
                    {
                    if (buffer[1] == 'b'){
                        // balance. correction tunings
                        port.println("$Stored balance tunings");
                        storeBalanceTunings();
                    }
                    else if (buffer[1] == 'p'){
                        // pos. correction tunings
                        port.println("$Stored pos. tunings");
                        storePosTunings();
                    }
                    else if (buffer[1] == 'i'){
                        // imu offset
                        port.println("$Stored IMU offset");
                        storeIMUOffset();
                    }
                    else if (buffer[1] == 's'){
                        // current state (i.e. which things are enabled)
                        port.println("$Stored current state");
                        storeState();
                    }
                    else if (buffer[1] == 'm'){
                        port.println("$Stored motor min power");
                        storeMotorMinPwr();
                    }
                    else if (buffer[1] == 'M'){
                        // magnetometer calibrations
                        port.println("$Stored mag calib");
                        storeMagCalib();
                    }
                    else{
                        // all of it
                        port.println("$Stored all of it");
                        storeBalanceTunings();
                        storePosTunings();
                        storeIMUOffset();
                        storeState();
                        storeMotorMinPwr();
                    } 
                    }
                    break;
                case 'r':
                    {
                    if (buffer[1] == 'b'){
                        // balance. correction tunings
                        port.println("$Read balance tunings");
                        readBalanceTunings();
                    }
                    else if (buffer[1] == 'p'){
                        // pos. correction tunings
                        port.println("$Read pos. tunings");
                        readPosTunings();
                    }
                    else if (buffer[1] == 'i'){
                        // imu offset
                        port.println("$Read IMU offset");
                        readIMUOffset();
                    }
                    else if (buffer[1] == 's'){
                        // current state (i.e. which things are enabled)
                        port.println("$Read previous state");
                        readState();
                    }
                    else if (buffer[1] == 'm'){
                        // motor min power
                        port.println("$Read motor min power");
                        readMotorMinPwr();
                    }
                    else if (buffer[1] == 'M'){
                        // magnetometer calibrations
                        port.println("$Read mag calib");
                        readMagCalib();
                    }
                    else{
                        // all of it
                        port.println("$Read all of it");
                        readMagCalib();
                        readBalanceTunings();
                        readPosTunings();
                        readState();
                        readIMUOffset();
                        readMotorMinPwr();
                    }
                    }
                    break;
                case 'v':
                	port.println(pgm_version);
                	break;
                default: 
                    // port.println("???");
                    break;
            }
        } 
        else{
            while (port.available()){
                incoming_c = port.read();                        
                switch(incoming_c){
                    // direct commands that don't require newline
                    case 'E':
                        {
                        port.println("$Enabled bController");
                        bController.enable();
                        }
                        break;
                    case '$':       bController.setTargV(0, 0);      break; //stop
                    case '<':       bController.setTargV(-100, 0);   break; //l
                    case '>':       bController.setTargV(100, 0);    break; //r
                    case '^':       bController.setTargV(0, 100);    break; //f
                    case '|':       bController.setTargV(0, -100);   break; //b
                    case '`':       bController.setTargV(-71, 71);   break; //fl
                    case (char)39:  bController.setTargV(71, 71);    break; //fr '''
                    case '/':       bController.setTargV(-71, -71);  break; //bl
                    case (char)92:  bController.setTargV(71, -71);   break; //br '\''
                    default:
                    {
                        buffer[cnt++] = incoming_c;
                        if ((incoming_c == '\n') || (cnt == sizeof(buffer)-1)){
                            buffer[cnt] = '\0';
                            cnt = 0;
                            ready = true;
                        }
                    } 
                    break;  
                }        
            }
        }
    }

    // appends character to output buffer
    void append(char c){
        outBuffer[outBuffIndex] = c;
        outBuffIndex++;

        if (outBuffIndex == outBuffSize){
            outBuffIndex = 0;
        }
    }

    // sends output buffer to port
    void send(){
        for (int i = 0; i < outBuffIndex; i++){
            port.print(outBuffer[i]);
            delayMicroseconds(10);
        }
        outBuffIndex = 0;
    }

    using Print::write;   // inherits Print (see core libraries)

    size_t write(uint8_t b){
        append((char)b);
        return 1;
    }
private:
    Stream &port;
    /* reading */
    char incoming_c;
    char buffer [64];
    int cnt = 0;
    bool ready = false;

    /* writing */
    static const int outBuffSize = 512;
    char outBuffer[outBuffSize];
    int outBuffIndex = 0;

    bool bcBalanceEnabled;
    bool bcPosCorEnabled;
};

SerialController btCtrl(BT);
SerialController serCtrl(Serial);

static void storeBalanceTunings(){
    EEPROM_writeAnything(200, bController.kp_theta);
    EEPROM_writeAnything(204, bController.ki_theta);
    EEPROM_writeAnything(208, bController.kd_theta);
    EEPROM_writeAnything(252, bController.kp_v);
    EEPROM_writeAnything(256, bController.ki_v);
    EEPROM_writeAnything(260, bController.kd_v);
}

static void readBalanceTunings(){
    float _kp_theta, _ki_theta, _kd_theta;
    float _kp_v, _ki_v, _kd_v;
    EEPROM_readAnything(200, _kp_theta);
    EEPROM_readAnything(204, _ki_theta);
    EEPROM_readAnything(208, _kd_theta);
    EEPROM_readAnything(252, _kp_v);
    EEPROM_readAnything(256, _ki_v);
    EEPROM_readAnything(260, _kd_v);
    bController.setTunings(
        _kp_theta,
        _ki_theta,
        _kd_theta,
        _kp_v,
        _ki_v,
        _kd_v,
        bController.kp_pos,
        bController.ki_pos,
        bController.kd_pos
    );
}

static void storePosTunings(){
    EEPROM_writeAnything(212, bController.kp_pos);
    EEPROM_writeAnything(216, bController.ki_pos);
    EEPROM_writeAnything(220, bController.kd_pos);
}
static void readPosTunings(){
    float _kp_pos, _ki_pos, _kd_pos;
    EEPROM_readAnything(212, _kp_pos);
    EEPROM_readAnything(216, _ki_pos);
    EEPROM_readAnything(220, _kd_pos);
    bController.setTunings(
        bController.kp_theta,
        bController.ki_theta,
        bController.kd_theta,
        bController.kp_v,
        bController.ki_v,
        bController.kd_v,
        _kp_pos,
        _ki_pos,
        _kd_pos
    );
}

static void storeState(){
    bool bcBalanceEnabled = bController.balanceEnabled();
    bool bcPosCorEnabled = bController.posCorEnabled();
    EEPROM_writeAnything(224, bcBalanceEnabled);
    EEPROM_writeAnything(228, bcPosCorEnabled);
    EEPROM_writeAnything(232, calibMotorMode);
    EEPROM_writeAnything(236, serDebug);
    EEPROM_writeAnything(240, btDebug);
}
static void readState(){
    bool bcBalanceEnabled, bcPosCorEnabled;
    EEPROM_readAnything(224, bcBalanceEnabled);
    EEPROM_readAnything(228, bcPosCorEnabled);
    EEPROM_readAnything(232, calibMotorMode);
    EEPROM_readAnything(236, serDebug);
    EEPROM_readAnything(240, btDebug);
    if (bcBalanceEnabled != bController.balanceEnabled()){
        if (bcBalanceEnabled){
            bController.enableBalance();
        }
        else{
            bController.disableBalance();
        }
    }
    if (bcPosCorEnabled != bController.posCorEnabled()){
        if (bcPosCorEnabled){
            bController.enablePosCorrection();
        }
        else{
            bController.disablePosCorrection();
        }
    }
}
static void storeIMUOffset(){
    EEPROM_writeAnything(244, roll_offset);
    EEPROM_writeAnything(248, pitch_offset);
}
static void readIMUOffset(){
    EEPROM_readAnything(244, roll_offset);
    EEPROM_readAnything(248, pitch_offset);
}

void zeroImu(){
    roll_offset = roll + roll_offset;
    pitch_offset = pitch + pitch_offset;
}

/* Magnetometer calibration stuff
 *   Need to calibrate the AK8963 for hard and soft iron distortion
 *   A simple method is implemented using the max/min readings of
 *   each axis
 */

void calibMagRoutine(){
    noInterrupts();

    uint16_t ii = 0, sample_count = 10000; // (100 seconds)

    mag_max[0] = 0xFFFF;    mag_min[0] = 0x7FFF;
    mag_max[1] = 0xFFFF;    mag_min[1] = 0x7FFF;
    mag_max[2] = 0xFFFF;    mag_min[2] = 0x7FFF;    

    // calibrate for up to 100 seconds or until serial receives data
    for(ii = 0; ii < sample_count; ii++) {
        digitalWriteFast(LED, !digitalReadFast(LED));   // fast blinking
        mpu.read_mag();

        for (int jj = 0; jj < 3; jj++) {
            if(mpu.mag_data_raw[jj] > mag_max[jj]) mag_max[jj] = mpu.mag_data_raw[jj];
            if(mpu.mag_data_raw[jj] < mag_min[jj]) mag_min[jj] = mpu.mag_data_raw[jj];
        }
        delay(10);  // AK8963 mode 2 => 100Hz
        if (Serial.available() || BT.available()) break;
    }
    CLEARSERIAL();
    CLEARBT();

    calcMagCalib();

    interrupts();
}

void calcMagCalib(){
    float mag_scale[3] = {0, 0, 0};

    // Get hard iron correction
    mag_bias[0]  = mpu.Magnetometer_ASA[0] * (mag_max[0] + mag_min[0])/2;  // get average x mag bias in counts
    mag_bias[1]  = mpu.Magnetometer_ASA[1] * (mag_max[1] + mag_min[1])/2;  // get average y mag bias in counts
    mag_bias[2]  = mpu.Magnetometer_ASA[2] * (mag_max[2] + mag_min[2])/2;  // get average z mag bias in counts

    // Get soft iron correction estimate
    mag_scale[0]  = mpu.Magnetometer_ASA[0] * (mag_max[0] - mag_min[0])/2;  // get average x axis max chord length in counts
    mag_scale[1]  = mpu.Magnetometer_ASA[1] * (mag_max[1] - mag_min[1])/2;  // get average y axis max chord length in counts
    mag_scale[2]  = mpu.Magnetometer_ASA[2] * (mag_max[2] - mag_min[2])/2;  // get average z axis max chord length in counts

    float avg_rad = mag_scale[0] + mag_scale[1] + mag_scale[2];
    avg_rad /= 3.0;

    mag_scalar[0] = avg_rad/((float)mag_scale[0]);
    mag_scalar[1] = avg_rad/((float)mag_scale[1]);
    mag_scalar[2] = avg_rad/((float)mag_scale[2]);
}

static void storeMagCalib(){
    int i = 0;
    for (; i < 3; i++){
        EEPROM_writeAnything(100 + 4*i, mag_max[i]);
    }
    for (; i < 6; i++){
        EEPROM_writeAnything(100 + 4*i, mag_min[i-3]);
    }
}

static void readMagCalib(){
    int i = 0;
    for (; i < 3; i++){
        EEPROM_readAnything(100 + 4*i, mag_max[i]);
    }
    for (; i < 6; i++){
        EEPROM_readAnything(100 + 4*i, mag_min[i-3]);
    }
    calcMagCalib();

    Serial.println("Read Mag Calib");
    Serial.print("mag_bias ");   PRINTARRAY(mag_bias);
    Serial.print("mag_scalar "); PRINTARRAY(mag_scalar);
}

static void storeMotorMinPwr(){
    EEPROM_writeAnything(265, motorMinPwr);
}

static void readMotorMinPwr(){
    EEPROM_readAnything(265, motorMinPwr);
}


/* Read IMU
 *
 */

void mpuRead(){
    // only read mag at 100Hz
    if (readMagDt > 10000){
        readMagDt = 0;
        mpu.read_all();
    }
    else{
        mpu.read_gyro();
        mpu.read_acc();
    }

    mpu.mag_data[0] = (mpu.mag_data[0] - mag_bias[0]) * mag_scalar[0];
    mpu.mag_data[1] = (mpu.mag_data[1] - mag_bias[1]) * mag_scalar[1];
    mpu.mag_data[2] = (mpu.mag_data[2] - mag_bias[2]) * mag_scalar[2];
}

/* Read Sensor fusion update
 *
 */

void Compass_Heading(){
    float mag_x;
    float mag_y;
    float cos_roll;
    float sin_roll;
    float cos_pitch;
    float sin_pitch;

    cos_roll = cos(roll);
    sin_roll = sin(roll);
    cos_pitch = cos(pitch);
    sin_pitch = sin(pitch);

    // Tilt compensated magnetic field X
    mag_x = mpu.mag_data[1] * cos_pitch + mpu.mag_data[0] * sin_roll * sin_pitch + mpu.mag_data[2] * cos_roll * sin_pitch;
    // Tilt compensated magnetic field Y
    mag_y = mpu.mag_data[0] * cos_roll - mpu.mag_data[2] * sin_roll;
    // Magnetic Heading
    float MAG_Heading = atan2(-mag_y, mag_x);
}
void fusionUpdate(){
    float readIMUdt_s = readIMUdt/1000000.0f;
    readIMUdt = 0;

    sensfusionUpdateQ(mpu.gyro_data[0],
                      mpu.gyro_data[1],
                      mpu.gyro_data[2],
                      mpu.accel_data[0],
                      mpu.accel_data[1],
                      mpu.accel_data[2],
                      // mpu.mag_data[1],
                      // mpu.mag_data[0],
                      // mpu.mag_data[2],
                      readIMUdt_s);
}

void fusionGetEuler(){
    sensfusionGetEulerRPY(&roll, &pitch, &yaw);
    roll -= roll_offset;
    pitch -= pitch_offset;

    Compass_Heading();
}

/* Poll encoders
 *
 */

inline void encoderPoll(){
    encState = (digitalReadFast(ENCXB)<<1) | (digitalReadFast(ENCXA))     
             | (digitalReadFast(ENCYB)<<5) | (digitalReadFast(ENCYA)<<4) 
             | (digitalReadFast(ENCZB)<<9) | (digitalReadFast(ENCZA)<<8);

    encIndex = encState | (lEncState << 2);

    /* byte   4       3    2    1
              lB   lA   B    A */

    // if anything the following 4 lines are the best to optimise
    rtA += enc_TAB[ (encIndex >> 0)  & 0x0f]; // we are only using 4 bytes per encoder
    rtB += enc_TAB[ (encIndex >> 4)  & 0x0f];
    rtC += enc_TAB[ (encIndex >> 8)  & 0x0f];

    lEncState = encState;
}

inline void encoderGetVelocity(){
    int dt = encUpdateDt;
    encUpdateDt = 0;

    wA = (rtA - prA) * 1000000 / dt / CPRAD;
    vA = wA * WHEEL_R;
    tA += wA * dt;

    wB = (rtB - prB) * 1000000/ dt / CPRAD;
    vB = wB * WHEEL_R;
    tB += wB * dt;

    wC = (rtC - prC) * 1000000 / dt / CPRAD;
    vC = wC * WHEEL_R;
    tC += wC * dt;

    prA = rtA; // cleanup
    prB = rtB;
    prC = rtC;
}

void calibMotorModeRoutine(){
    if (calibMotorModeDt >= 100000){
        calibMotorModeDt = 0;
        if (pA == calibMotorModeAmplitude){
            calibMotorModeDir = false;
        }
        else if (pA == -calibMotorModeAmplitude){
            calibMotorModeDir = true;
        }
        if (calibMotorModeDir){
            pA++;
            pB++;
            pC++;
        }
        else{
            pA--;
            pB--;
            pC--;
        }
    }
    BT.print(calibMotorModeDt); BT.print('\t');
    BT.print(pA); BT.print('\t');
    BT.print(wA); BT.print('\t');
    BT.print(wB); BT.print('\t');
    BT.print(wC); BT.println();

    Serial.print(calibMotorModeDt); Serial.print('\t');
    Serial.print(pA); Serial.print('\t');
    Serial.print(wA); Serial.print('\t');
    Serial.print(wB); Serial.print('\t');
    Serial.print(wC); Serial.println();
}