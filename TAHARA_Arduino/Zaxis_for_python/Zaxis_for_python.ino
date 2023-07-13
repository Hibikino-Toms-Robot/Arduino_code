#include <Stepper.h>

/*

COM13

receive:
        data
                -data[0]: 'I'→Initialize_mode 'T'→Target_mode
                -data[1]: アーム移動位置の10の位
                -data[2]: アーム移動位置の1の位
                -data[3]: アーム移動位置の1/10の位
                -data[4]: 前回のアームの位置の10の位
                -data[5]: 前回のアームの位置の1の位
                -data[6]: 前回のアームの位置の1/10の位
                -data[7]: 入力終了判定の','
        
send:
        Finish_flag
                -INIT_finish: 初期化完了
                -Target_finish: 目標位置へ移動完了
*/

const int cwA=3;
const int cwB=2;
const int ccwA=6;
const int ccwB=5;

const int sw_d=7;

/* stepsPerRevolution:モータの1回転あたりのステップ数(実験的に調整)
 * これによってsetSpeedのrpmが決まる*/
const int stepsPerRevolution=800;
Stepper myStepperCW(stepsPerRevolution,cwA,cwB);
Stepper myStepperCCW(stepsPerRevolution,ccwA,ccwB);

float target; // z軸目標位置
float preLocation;//絶対座標
long targetStep;
long targetStep_sub;
int cwSpeed=580;//モータ回転速度[rpm]
int initialSpeed=200;//モータ回転速度[rpm]、初期速度


uint8_t count = 0;
char data[32];

void setup() {
  pinMode(sw_d, INPUT);
  pinMode(cwA, OUTPUT);
  pinMode(cwB, OUTPUT);
  pinMode(ccwA, OUTPUT);
  pinMode(ccwB, OUTPUT);
  Serial.begin(115200);
}

float Conversion(int num1, int num2, int num3){
  // 目標位置計算
  // 百の位
  // int Num1 = num1 - '0';      // char型をint型に変換
  // if (Num1 > 0 && Num1 < 10){
  //   Num1 = Num1*100;          
  // } else Num1 = 0;
  
  // 十の位
  int Num1 = num1 - '0';      
  if (Num1 > 0 && Num1 < 5){
    Num1 = Num1*10;           
  } else Num1 = 0;
  
  // 一の位
  int Num2 = num2 - '0';
  if (Num2 > 0 && Num2 < 10){
    Num2 = Num2*1;            
  } else Num2 = 0;

  // 1/10の位
  int Num3 = num3 - '0';
  if (Num3 > 0 && Num3 < 10){
    Num3 = Num3*1;            
  } else Num3 = 0;
  
  float Num = Num1 + Num2 + Num3*0.1; // 合計

  return Num;
}

int sendPulse(float target, float preLocation){
  // rpm設定
  myStepperCW.setSpeed(cwSpeed);
  myStepperCCW.setSpeed(cwSpeed);
  target = target;
  if(preLocation>target){
    long targetStep=(preLocation-target)*2000;
    long targetStep_sub=targetStep;
    if(targetStep_sub>0){

      // myStepperCW.stepの引数はint型であり、32767を超えるとオーバーフローしてしまうので、その対策
      while(targetStep_sub - 32767 > 0){
        targetStep_sub = targetStep_sub - 32767;
        myStepperCW.step(32767);
      }      
      myStepperCW.step(targetStep_sub);
    }
    else{
      myStepperCW.step(targetStep);
    }
    preLocation=target;
  }

  else if(preLocation<target){
    long targetStep=(target-preLocation)*2000;
    long targetStep_sub=targetStep;
    if(targetStep_sub>0){

      while(targetStep_sub - 32767 > 0){
        targetStep_sub = targetStep_sub - 32767;
        myStepperCCW.step(32767);
      }
      myStepperCCW.step(targetStep_sub);
    }
    else{
      myStepperCCW.step(targetStep);
    }
    preLocation=target;
  }
  else{
    preLocation=target;
  }
}

void Initialize(float target){
  // rpm設定
  myStepperCW.setSpeed(initialSpeed);
  myStepperCCW.setSpeed(initialSpeed);

  while(digitalRead(sw_d) == HIGH){
    myStepperCW.step(250);
    // delay(1)を入れることで、確実にリミットスイッチまで移動させるようにする。
    // 代償として、異音が発生する。
    delay(2);
  }
  
  preLocation = 0.0;

  delay(250);  
  sendPulse(target, preLocation);
}

void loop() {
  if(Serial.available()){
    data[count] = Serial.read();
    if(data[count] == ','){

      float target = Conversion(data[1], data[2], data[3]);
      float preLocation = Conversion(data[4], data[5], data[6]);

      // I: Initialize
      if(data[0] == 'I'){
        // 動作
        Initialize(target);
        // 動作終了後
        Serial.println("INIT_finish");
      }

      // T: Move target position
      else if(data[0] == 'T'){
        // 動作
        sendPulse(target, preLocation);
        // 動作終了後
        Serial.println("Target_finish");
      }
      count = 0;
    }else{
      count ++;
    }    
  }  
}