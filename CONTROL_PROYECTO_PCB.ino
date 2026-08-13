#include <Bluepad32.h>


// PINES MOTOR A

const int PWMA = 25;
const int AIN1 = 27;
const int AIN2 = 26;


// PINES MOTOR B 

const int PWMB = 13;
const int BIN1 = 32;
const int BIN2 = 14;


// PIN PARA STBY

const int STBY = 33;

// LED 
const int LED_PIN = 4;

// CONFIGURACIÓN DEL CONTROL


// Zona muerta del joystick.
// Cualquier valor entre -40 y +40 se considera 0.
const int JOYSTICK_DEADZONE = 40;

// Zona muerta de los gatillos.
const int TRIGGER_DEADZONE = 30;

// CONTROLADOR CONECTADO

ControllerPtr myController = nullptr;

// PROTOTIPOS DE FUNCIONES
void setMotorA(int speed);
void setMotorB(int speed);
void stopMotors();

// CONTROLADOR CONECTADO

void onConnectedController(ControllerPtr ctl) {

  // Solo acepta un controlador
  if (myController == nullptr) {

    myController = ctl;

    Serial.println();
    Serial.println("================================");
    Serial.println("CONTROL PS4 CONECTADO");
    Serial.println("================================");
    Serial.println();

    // Encender LED
    digitalWrite(LED_PIN, HIGH);

  } else {

    Serial.println("Ya existe un controlador conectado.");
  }
}


// CONTROLADOR DESCONECTADO


void onDisconnectedController(ControllerPtr ctl) {

  if (myController == ctl) {

    Serial.println();
    Serial.println("================================");
    Serial.println("CONTROL PS4 DESCONECTADO");
    Serial.println("================================");
    Serial.println();

    // Eliminar referencia al control
    myController = nullptr;

    // Apagar LED
    digitalWrite(LED_PIN, LOW);

    // Detener inmediatamente los motores
    stopMotors();
  }
}


// MOTOR A


void setMotorA(int speed) {

  // Limitar el valor al rango de -255 hasta +255
  speed = constrain(speed, -255, 255);

  // AVANCE

  if (speed > 0) {

    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    analogWrite(PWMA, speed);
  }

  // RETROCESO

  else if (speed < 0) {

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);

    // Convertir número negativo a positivo
    analogWrite(PWMA, -speed);
  }

  // DETENER

  else {

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);

    analogWrite(PWMA, 0);
  }
}

// MOTOR B

void setMotorB(int speed) {

  // Limitar el valor al rango de -255 a +255
  speed = constrain(speed, -255, 255);

  // AVANCE

  if (speed > 0) {

    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);

    analogWrite(PWMB, speed);
  }

  // RETROCESO

  else if (speed < 0) {

    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);

    analogWrite(PWMB, -speed);
  }

  // DETENER

  else {

    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);

    analogWrite(PWMB, 0);
  }
}

// DETENER MOTORES

void stopMotors() {

  setMotorA(0);
  setMotorB(0);
}

// SETUP

void setup() {

  // INICIAR MONITOR SERIAL

  Serial.begin(115200);

  delay(1000);

  // CONFIGURACIÓN DE PINES

  // Motor A
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  // Motor B
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  // TB6612FNG
  pinMode(STBY, OUTPUT);

  // LED
  pinMode(LED_PIN, OUTPUT);

  // ESTADO INICIAL

  // LED apagado
  digitalWrite(LED_PIN, LOW);

  // TB6612 en standby
  digitalWrite(STBY, LOW);

  // Motores detenidos
  stopMotors();

  // ACTIVAR TB6612FNG

  digitalWrite(STBY, HIGH);

  // INICIAR BLUEPAD32

  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );


  // Desactivar dispositivo virtual
  BP32.enableVirtualDevice(false);

  // MENSAJE DE INICIO

  Serial.println();
  Serial.println("COYOTE TANGO");
  Serial.println();

  Serial.println("Sistema iniciado.");
  Serial.println("Esperando control PS4");
  Serial.println();

}

// LOOP
void loop() {

  // ACTUALIZAR INFORMACIÓN DE BLUEPAD32

  bool dataUpdated = BP32.update();

  // COMPROBAR SI HAY UN CONTROL CONECTADO

  if (dataUpdated && myController != nullptr) {

    ControllerPtr ctl = myController;

    // LECTURA DE GATILLOS 

    // Valor ORIGINAL recibido desde Bluepad32
    //
    // aproximadamente:
    // 0 = suelto
    // 1023 = completamente presionado

    int rawThrottle = ctl->throttle();
    int rawBrake = ctl->brake();

    // SE COPIAR VALORES PARA PROCESAMIENTO

    int throttle = rawThrottle;
    int brake = rawBrake;

    // ZONA MUERTA GATILLOS
 

    if (throttle < TRIGGER_DEADZONE) {
      throttle = 0;
    }

    if (brake < TRIGGER_DEADZONE) {
      brake = 0;
    }

    // CONVERTIR 0-1023 A 0-255

    throttle = map(
      throttle,
      0,
      1023,
      0,
      255
    );

    brake = map(
      brake,
      0,
      1023,
      0,
      255
    );

    // CALCULO DEL AVANCE Y RETROCESO

    /*
       R2 > L2  → avance
       L2 > R2  → retroceso
       R2 = L2  → detenido
    */

    int throttleCommand = throttle - brake;

    // LECTURA DEL JOYSTICK DERECHO

    // axisRX() = eje horizontal del joystick derecho

    int rawSteering = ctl->axisRX();

    int steering;

    // ZONA MUERTA JOYSTICK

    if (abs(rawSteering) < JOYSTICK_DEADZONE) {

      steering = 0;

    } else {

      // CONVERSIÓN

      steering = map(
        rawSteering,
        -511,
        512,
        -255,
        255
      );
    }

    // SISTEMA DIFERENCIAL DE VELOCIDADES

    /*
        VELOCIDAD BASE = throttleCommand

        GIRO = steering

        Motor A = velocidad + giro
        Motor B = velocidad - giro
    */

    int leftMotor =
      throttleCommand + steering;

    int rightMotor =
      throttleCommand - steering;

    // LIMITACIÓN DE LOS VALORES DE LOS MOTORES

    leftMotor = constrain(
      leftMotor,
      -255,
      255
    );

    rightMotor = constrain(
      rightMotor,
      -255,
      255
    );

    // ENVÍO DE INFORMACIÓN AL MOTOR A 

    setMotorA(leftMotor);

    // ENVÍO DE INFORMACIÓN AL MOTOR B 

    setMotorB(rightMotor);

    // APARICIÓN DE DATOS EN EL MONITOR SERIAL

    Serial.print("R2 raw: ");
    Serial.print(rawThrottle);

    Serial.print(" | L2 raw: ");
    Serial.print(rawBrake);

    Serial.print(" | RX raw: ");
    Serial.print(rawSteering);

    Serial.print(" | Velocidad: ");
    Serial.print(throttleCommand);

    Serial.print(" | Giro: ");
    Serial.print(steering);

    Serial.print(" | Motor A: ");
    Serial.print(leftMotor);

    Serial.print(" | Motor B: ");
    Serial.println(rightMotor);
  }

  // DELAY

  delay(5);
}
