/*
input:
        data
                -data[0]: num
                          0:Wait(Finger_Open)
                          1:Suction_On（吸い上げる）
                          2:Finger_Close（EEを閉じる）
                          3:Finger_Open（EEを開く）
                          4:Suction_Off（吸い上げ止め）
                -data[1]: EE_PWM 100の位
                -data[2]: EE_PWM 10の位
                -data[3]: EE_PWM 1の位
                -data[4]: 入力終了判定の','
        
output:
        flag:
                Waitのとき
                    0:初期化完了

                Suction_Onのとき
                    1:光センサが未反応
                    5:光センサが反応（中にトマトが入っている）

                Finger_Closeのとき
                    2:上SW反応（EEが閉じた状態）

                Finger_Openのとき
                    3:下SW反応（EEが開いた状態）
                    6:光センサ反応（EE内にトマトが存在）
                    7:光センサ（吸引終了、トマト落下確認）

                Suction_Offのとき
                    4:吸引動作終了
*/



// Parameter //////////////////////////////////////////////////////////
#define PWM 5                       // cutter動作用モータのPWM
#define A 3                         // Aピン                         
#define B 4                         // Bピン
#define TSW 6                       // Close_cutter_SW
#define BSW 7                       // Open_cutter_SW
#define OPTICAL 1                   // 光センサ
#define EDF 9                       // EDFのPWM

int optical_sensor = 0;             // 光センサの値:Analog
int optical_sensor_cali[10];        // 光センサのしきい値のキャリブレーション用
int sensor_threshold = 0;           // 光センサのしきい値の初期化
int top_sw = 1;                     // TSW(Top_SW)
int bottom_sw = 1;                  // BSW(Bottom_SW)

int InitEDF_PWM = 100;              // EDF起動時のPWM
int EDF_PWM;                        // EDF吸引時のPWM(serialで受信)

uint8_t receive_data;               // Pythonから受信したデータ
uint8_t mode;                       // EEのモード番号

uint8_t send_data;                  // Simulinkに送信するデータ
uint8_t flag = 0;                   // EEの状態
uint8_t sensor = 0;                 // 光センサの値

int count = 0;
char data[32];

// Function ///////////////////////////////////////////////////////////

int Conversion(int num1, int num2, int num3){
  // 百の位
  int Num1 = num1 - '0';      // char型をint型に変換
  if(Num1 > 0 && Num1 <= 2){
    Num1 = Num1*100;          
  }else if(Num1 > 2){
    Num1 = 200;
  }else{
    Num1 = 0;
  }

  // 十の位
  int Num2 = num2 - '0';
  if(Num1 >= 200){
    if(Num2 > 5 && Num2 < 10){
      Num2 = 50;
    }else{
      Num2 = Num2*10;
    }
  }else if(Num2 > 0 && Num2 < 10){
    Num2 = Num2*10;
  }else{
    Num2 = 0;
  }
  
  // 一の位
  int Num3 = num3 - '0';
  if(Num1 >= 200 && Num2 >= 50){
    if(Num3 > 5 && Num3 < 10){
      Num3 = 5;
    }else{
      Num3 = Num3*1;
    }
  }else if(Num3 > 0 && Num3 < 10){
    Num3 = Num3*1;
  }else{
    Num3 = 0;
  }
  
  int Num = Num1 + Num2 + Num3; // 合計
  return Num;
}

void Send(){                        
  Serial.println(flag);
}

void setup() {                      // 初期化とPin番号
  pinMode(A, OUTPUT);               // モータドライバ Aピン
  pinMode(B, OUTPUT);               // モータドライバ Bピン
  pinMode(PWM, OUTPUT);             // cutter動作用モータのPWM出力
  pinMode(TSW, INPUT_PULLUP);       // Close_cutter
  pinMode(BSW, INPUT_PULLUP);       // Open_cutter
  pinMode(OPTICAL, INPUT);          // 光センサ
  pinMode(EDF, OUTPUT);             // EDF用PWM出力

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  // モータ停止
  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);
  analogWrite(PWM, 0);
  analogWrite(EDF, InitEDF_PWM);
  Serial.begin(115200);

  // 光センサから10個のデータ取得
  int cnt = 0;  // カウント変数
  for(cnt = 0 ; cnt < 10 ; cnt++){
    optical_sensor_cali[cnt] = analogRead(OPTICAL); //光センサの値取得
  }
  // 光センサ10のデータの平均値
  int optical_sensor_sum = 0; //10個の光センサデータの合計変数の初期化
  double optical_sensor_mean = 0; //光センサの平均値変数の初期化
  cnt = 0;
  for(cnt = 0 ; cnt < 10 ; cnt++){
    optical_sensor_sum += optical_sensor_cali[cnt];  //10個の光センサデータの合計
  }
  optical_sensor_mean = (double)optical_sensor_sum / (double)(cnt + 1); // 光センサの平均値
  double t = 0; //偏差変数の初期化
  double optical_sensor_std = 0; //標準偏差変数の初期化
  cnt = 0;
  for(cnt = 0 ; cnt < 10 ; cnt++){
    t += (double)optical_sensor_cali[cnt] - (double)optical_sensor_mean; //偏差
  }
  optical_sensor_std = sqrt(t / (double)(cnt + 1)); //標準偏差
  if(optical_sensor_std > 100){ //分散が大きすぎる場合
    sensor_threshold = 900;
  }
  else{ //分散が許容範囲である場合
    sensor_threshold = (int)optical_sensor_mean + 50 + 40; //平均値 + 50 + 40をしきい値とする
  }  
}


void loop(){

  //モータ停止
  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);

  if(Serial.available() > 0){
    data[count] = Serial.read();
    if(data[count] == ','){
      mode = data[0] - '0';
      EDF_PWM = Conversion(data[1], data[2], data[3]);

      flag = 0;
      // optical_sensor = analogRead(OPTICAL);

      //sensor = round(optical_sensor / 20) - 41;
      sensor = round((optical_sensor-700)/10);
      if(sensor < 0){
        sensor = 0;
      }else if(sensor > 15){
        sensor = 15;
      }

      switch(mode){

        case 0:  //Wait
          flag = 0;
          analogWrite(EDF, InitEDF_PWM);
          digitalWrite(A, HIGH);
          digitalWrite(B, LOW);
          while(digitalRead(BSW)){
            analogWrite(PWM, 150);
          }
          analogWrite(PWM, 0);
          digitalWrite(A, HIGH);
          digitalWrite(B, HIGH);
          analogWrite(EDF, InitEDF_PWM);
          Send();
          break;
        
        case 1:  // Suction_On
          flag = 1;
          analogWrite(EDF, EDF_PWM);
          digitalWrite(A, HIGH);
          digitalWrite(B, HIGH);
          analogWrite(PWM, 0);
          optical_sensor = analogRead(OPTICAL);
          if(optical_sensor > sensor_threshold){
            // Serial.print(optical_sensor);  Serial.print(" "); Serial.println(sensor_threshold);
            flag = 5;
          }
          analogWrite(PWM, 0);
          digitalWrite(A, HIGH);
          digitalWrite(B, HIGH);
          analogWrite(EDF, EDF_PWM);
          Send();
          break;

        case 2:  //Finger_Close
          flag = 2;
          analogWrite(EDF, EDF_PWM);
          digitalWrite(A, LOW);            
          digitalWrite(B, HIGH);
          while(digitalRead(TSW)){         
            analogWrite(PWM, 250);
          }
          analogWrite(PWM, 0);
          digitalWrite(A, HIGH);            
          digitalWrite(B, HIGH);
          analogWrite(EDF, EDF_PWM);
          Send();
          break;

        case 3:  //Finger_Open
          flag = 3;
          analogWrite(EDF, EDF_PWM);
          digitalWrite(A, HIGH);            
          digitalWrite(B, LOW);
          while(digitalRead(BSW)){         
            analogWrite(PWM, 120);
          }
          optical_sensor = analogRead(OPTICAL);
          if(optical_sensor > sensor_threshold){
            flag = 6; //トマトが中に入っているか
          }else{
            flag = 7; //トマトが落ちたことが確認
          }
          analogWrite(PWM, 0);
          digitalWrite(A, HIGH);            
          digitalWrite(B, HIGH);
          analogWrite(EDF, InitEDF_PWM);
          Send();
          break;

        case 4:  //Suction_Off
          flag = 4;
          analogWrite(EDF, InitEDF_PWM);
          digitalWrite(A, HIGH);
          digitalWrite(B, HIGH);
          analogWrite(PWM, 0);
          Send();
          break;
      }
    }else{
      count ++;
    }
  }
  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);
}