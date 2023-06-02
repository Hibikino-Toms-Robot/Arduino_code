//Pin番号定義
#define motorEnabled 11
#define runbrake 10
#define motorDirection 9
#define pwm 5
#define encoder 1
//使っていないpin
#define AlarmReset 7
#define INT 8


#define WHEEL_DIAMETER 0.05 // m // モーターの直径
#define WHEEL_TREAD 0.25 // m // 車輪のトレッド幅
#define ENC_COUNTS 30 // エンコーダーのカウント数
// エンコーダーの状態
int encoderA = 0;
int encoderPos = 0;
int lastEncoderPos = 0; 
float forward = 1.0; // 前進方向

// 現在の自己位置
float x = 0.0;
volatile int pulse = 0;     //エンコーダで数えたパルス数
String reciev_str ; //受信データ 

void setup() {
  pinMode(motorEnabled, OUTPUT);     //H:START L:STOP
  pinMode(runbrake, OUTPUT);  //H:RUN L:BREAK(Instant stop)
  pinMode(motorDirection, OUTPUT);     //H:Right L:Left)  
  pinMode(pwm, OUTPUT);       //This pin is Analog pin. Output 0～5V.
  //最初はモータをストップさせておく
  digitalWrite(motorEnabled, HIGH);
  digitalWrite(runbrake, HIGH);

  //使っていないpin
  //アラーム系
  pinMode(AlarmReset, OUTPUT);
  digitalWrite(AlarmReset, HIGH);
  //モータの制御状況がわかるらしいが使わない
  digitalWrite(INT, HIGH);

  Serial.begin(115200); // シリアル通信を開始する
  attachInterrupt(encoder, RecognizeRotation, RISING); //FALLING);  //割り込み処理定義
}

//パルスの立下り数を検出
void RecognizeRotation(void){
  pulse++;
}


//移動距離の計算
void estimatePosition(void){
  encoderPos=pulse;
  int encoderDiff = encoderPos - lastEncoderPos;
  float distance = forward * (encoderDiff * 2.0 * 3.14159 * (WHEEL_DIAMETER / 2.0)) / ENC_COUNTS;   // 車輪の移動距離を計算する
  x += distance; // 新しい自己位置を計算する
  lastEncoderPos = encoderPos;
  Send_Data(x);
}

//データ送信プログラム
void Send_Data(float distance){
  Serial.println(distance);
  // byte x_bytes[sizeof(x)];
  // memcpy(x_bytes, &x, sizeof(x));
  // Serial.write(x_bytes, sizeof(x)); 
}


//モータ正回転
void Forward_motor(int vel){
  digitalWrite(motorEnabled,LOW);  //運転
  digitalWrite(runbrake,LOW);  //New circuit
  digitalWrite(motorDirection,HIGH);  //正転
  analogWrite(pwm, abs(vel)); //速度指令
}

//モータ逆回転
void Reverse_motor(int vel){
  digitalWrite(motorEnabled,LOW);  //運転
  digitalWrite(runbrake,LOW);  //New circuit
  digitalWrite(motorDirection,LOW);     //逆転
  analogWrite(pwm, abs(vel)); //速度指令
}

//モータ停止
void Stop_motor(void){
  digitalWrite(motorEnabled,HIGH);   //速度0
  digitalWrite(runbrake,HIGH);   
  analogWrite(pwm, 0); 
}

//モータ瞬時停止
void InstantStop_motor(void){
  //瞬時停止
  digitalWrite(runbrake,HIGH); 
  digitalWrite(motorEnabled,HIGH); 
  analogWrite(pwm, 0); //速度0
}

/* 
 * 停止は減速の立下りが加速の立上りとほぼ同じ挙動をする
 * そのため、速度-時間曲線は等脚台形になる(ハズ)
 * それに対し瞬時停止はほぼ90度の角度で立下る
 * 挙動についてはモータの取扱説明書または引継資料を参照のこと
 */

//速度制御
void Control_Motor(String *cmd_vel){
  forward = cmd_vel[0].toFloat();
  float velocity = cmd_vel[1].toFloat();
  //正転
  if(velocity>0 &&  velocity<128){
    Forward_motor(velocity);
  }
  //逆転
  else if(velocity<0 && velocity>-127){
    Reverse_motor(velocity);
  }
  //停止
  else{
    Stop_motor();
  }
}




int split(String data, char delimiter, String *dst) {
  int index = 0;
  int arraySize = (sizeof(data) / sizeof((data)[0]));
  int datalength = data.length();
  for (int i = 0; i < datalength; i++) {
    char tmp = data.charAt(i);
    if (tmp == delimiter) {
      index++;
      if (index > (arraySize - 1)) return -1;
    }
    else dst[index] += tmp;
  }
  return (index + 1);
}


void recieve_cmd() {
  Serial.println("receive_cmd"); //文字列型で価を受け取る(例)"100,100"
  reciev_str = Serial.readStringUntil('\n');
  if (reciev_str.length() > 0) {
    String sp_reciev_str[2];
    split(reciev_str, ',', sp_reciev_str); //コンマで左右のrpmを区別する
    for (int i = 0 ; i < 2 ; i ++ ) {
      Serial.println(sp_reciev_str[i]);
    }
    Control_Motor(sp_reciev_str);
  }
}


//こっちのほうが賢いような気がしている
// void recieve_cmd() {
//   if (Serial.available()) { // データが受信されたかどうかを確認する
//     String data = Serial.readStringUntil('\n'); // 改行文字までの文字列を読み取る
//     Serial.println(data); // 受信したデータをシリアルモニタに出力する（確認用）
//     int commaIndex = data.indexOf(","); // カンマの位置を検索する
//     if (commaIndex >= 0) { // カンマが見つかった場合
//       String aString = data.substring(0, commaIndex); // 先頭からカンマまでの文字列を切り出す
//       String bString = data.substring(commaIndex + 1); // カンマ以降の文字列を切り出す
//       int a = aString.toInt(); // aを数値に変換する
//       int b = bString.toInt(); // bを数値に変換する
//       // a, bに対する処理を実行する
//     }
//   }
// }

//メインプログラム
void loop() {
  if(Serial.available())
  {
    recieve_cmd();
  }
  estimatePosition();
}