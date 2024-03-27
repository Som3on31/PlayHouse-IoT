// constants for printing lines
const char LINE[]= "-------------------------------------------";

// operation modes
const String pullMode = "PULL";
const String confirmMode = "CONF";
const String sendMode = "SEND";
const String sendSpecificMode = "SENF";
const String changeMode = "CHANGE";
String currentMode = "";

// string buffer for receiving msg from the game
const int txtBufferSize = 128;
char txtBuffer[txtBufferSize];
int txtLength = 0;
bool newLine = false;

// any changes to the IoT by the game will have a timer cooldown before it could work at its own will again
const unsigned long changeCooldown = 1800000;
const unsigned long interval = 1000UL;
unsigned long timer = 0;
unsigned long timerStartAt = 0;
unsigned long timerEndsAt = 0;
bool changeCooldownTimerStart = false;


class Room{
  public:
    Room(String name,int lightPin){
      this->name = ROOM + name;
      this->lightPin = lightPin;

      pinMode(this->lightPin,INPUT);
    }

    Room(){
    }

    // -- methods for the counter modules
    void incrementPeopleCount(){
      peopleInside++;
    }

    void decrementPeopleCount(){
      if (peopleInside>0) peopleInside--;
    }

    int peopleCount(){
      return peopleInside;
    }

    // -- for assigning names of the room
    void setName(String name){
      this->name = name;
    }

    String getName(){
      return name;
    }

    void setLight(bool status){
      roomLightStatus = status;
      digitalWrite(lightPin, status);
    }

    bool light(){
      return roomLightStatus;
    }

    // -- overloaded operators
    // Room& Room::operator=(const Room&){

    // }

  private:
    int peopleInside = 0;
    int lightPin;
    bool roomLightStatus = false;


    // Optional
    String name;
    const String ROOM = "Room ";
};

// A class specially for people counters
class CounterModule{
  public:
    CounterModule(int in,int out){
      outputPin_in = in;
      outputPin_out = out;

      pinMode(outputPin_in, INPUT);
      pinMode(outputPin_out, INPUT);
    }

    CounterModule(){
      outputPin_in = 0;
      outputPin_in = 1;

      pinMode(outputPin_in, INPUT);
      pinMode(outputPin_out, INPUT);
    }

    void checkInOut(){
      int currentInStatus = digitalRead(outputPin_in);
      int currentOutStatus = digitalRead(outputPin_out);


      //detection logic
      if((gettingIn() || gettingOut()) && !currentlyInOut){
        currentlyInOut = true;
      }
      else if(gettingOut() && stillIn() && currentlyInOut){
        // currentPeopleInside++;
        if (inside!=NULL) inside->incrementPeopleCount();
        if (outside!=NULL) outside->decrementPeopleCount();
        // printStatusDebug();
        currentlyInOut = false;
      }
      else if(gettingIn() && stillOut() && currentlyInOut){
        // if (currentPeopleInside>0) currentPeopleInside--;
        if (inside!=NULL) inside->decrementPeopleCount();
        if (outside!=NULL) outside->incrementPeopleCount();
        // printStatusDebug();
        currentlyInOut = false;
      }

      
      //update current status
      inStatus = currentInStatus;
      outStatus = currentOutStatus;
    }

    void setName(String name){
      this->name = name;
    }

    void setInside(Room* i){
      inside = i;
    }

    void setOutside(Room* o){
      outside = o;
    }

    void setInsideOutside(Room *i,Room *o){
      setInside(i);
      setOutside(o);
    }
    // int getPeopleCount(){
    //   return currentPeopleInside;
    // }


  private:
    //Ports for in and out
    int outputPin_in;
    int outputPin_out;

    //Sensor status
    int inStatus = 0;
    int outStatus = 0;
    bool currentlyInOut = false;

    // Detect how many people inside
    // int currentPeopleInside = 0;
    Room *inside;
    Room *outside;

    // Optional: name to identify where the module is installed in which room.
    String name;

    // check sensor status
    // LOW = near sensor
    // HIGH = far from the sensor
    bool notInYet(){
      return inStatus == HIGH && digitalRead(outputPin_in) == HIGH;
    }

    bool notOutYet(){
      return outStatus == HIGH && digitalRead(outputPin_out) == HIGH;
    }

    bool stillIn(){
      return inStatus == LOW && digitalRead(outputPin_in) == LOW;
    }

    bool stillOut(){
      return outStatus == LOW && digitalRead(outputPin_out) == LOW;
    }

    bool gettingIn(){
      return inStatus == HIGH && digitalRead(outputPin_in) == LOW;
    }

    bool gettingOut(){
      return outStatus == HIGH && digitalRead(outputPin_out) == LOW;
    }

    // void printStatusDebug(){
    //   Serial.println(LINE);
    //   if (!name.length() < 1) Serial.println(name);
    //   Serial.print("People: ");
    //   Serial.println(currentPeopleInside);
    //   Serial.print("in: ");
    //   Serial.print(digitalRead(outputPin_in));
    //   Serial.print(" ");
    //   Serial.println(inStatus);
    //   Serial.print("out: ");
    //   Serial.print(digitalRead(outputPin_out));
    //   Serial.print(" ");
    //   Serial.println(outStatus);
    //   Serial.print("currently in/out: ");
    //   Serial.println(!currentlyInOut ? "True" : "False");
    //   Serial.println(LINE);
    // }
};

// initialize rooms here
bool doorStatus[] = {false, false, false};

// initialize counters here
Room rooms[] = {Room("1",2),Room("2",3),Room("3",4)};
CounterModule counters[] = {CounterModule(8,9),
                            CounterModule(10,11),
                            CounterModule(12,13)};

unsigned long generalTimer = 0;
unsigned long previousGeneralTimer = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(500);
  // Serial.println(LINE);
  // Serial.println("Playhouse IoT V.0.0.2");
  // Serial.println(LINE);
  // Serial.println();
  // Serial.println();
  // Serial.println();
  counters[0].setInside(&rooms[0]);
  counters[1].setInsideOutside(&rooms[1],&rooms[0]);
  counters[2].setInsideOutside(&rooms[2],&rooms[0]);

  // delay(3000);
  // Serial.println("System ready.");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - generalTimer >= interval/10) {
    checkInOutAll();
  }
  // delay(20);
  receiveMsg();
  if (newLine) {
    // Serial.println("new line received");
    parseAndExecuteMsg();  
  }
  

  // timer related code
  if (millis() - timer >= interval && changeCooldownTimerStart){
    timer = millis();
  }
  if (timer > timerEndsAt){
    changeCooldownTimerStart = false;
  }
  if(!changeCooldownTimerStart){
    checkAllRooms();
    // Serial.println("work as usual");
  }

  

  // // debug only
  // if (millis() - generalTimer >= interval){
  //   Serial.println(LINE);
  //   for (int i=0;i<sizeof(rooms)/sizeof(Room);i++){
      
  //     Serial.println("Room " + rooms[i].getName());
  //     Serial.println("People inside: " + String(rooms[i].peopleCount()));
  //     Serial.println("Light: " + (rooms[i].light() ? String("On") : String("Off")));
  //   }
  //   Serial.println(LINE);
  //   generalTimer = millis();
  // }
  
  // Serial.println(changeCooldownTimerStart ? "Timer set" : "Timer not set");
  
  // if(changeCooldownTimerStart){
  //   Serial.println("Timer: " + String(timer));
  //   Serial.println("Timer ends at " + String(timerEndsAt));
  // }
}

void checkInOutAll(){
  for (int i=0;i<sizeof(counters)/sizeof(CounterModule);i++){
    counters[i].checkInOut();
    // Serial.println("checking");
  }
}

void checkAllRooms(){
  int currentPeople[sizeof(counters)/sizeof(CounterModule)];
  int roomNumber[sizeof(counters)/sizeof(CounterModule)];

  for(int i=0;i<sizeof(currentPeople)/sizeof(int);i++){
    currentPeople[i] = rooms[i].peopleCount();
    roomNumber[i] = i;
  }

  // then sort for median
  for(int i=0;i<sizeof(counters)/sizeof(CounterModule);i++){
    for (int j=0;j<(sizeof(counters)/sizeof(CounterModule))-1;j++){

      if(currentPeople[j]>currentPeople[j+1]){
        swap(currentPeople[j],currentPeople[j+1]);
        swap(roomNumber[j],roomNumber[j+1]);
      }else{
        break;
      }
    }
  }

  int middlePoint = sizeof(counters)/sizeof(CounterModule)/2;
  int median = middlePoint%2 == 1 ? currentPeople[middlePoint] : (currentPeople[middlePoint-1] + currentPeople[middlePoint]) / 2;
  

  // after getting the median, change the room light status using the median
  for(int i=0;i<sizeof(counters)/sizeof(CounterModule);i++){
    // roomsLightStatus[roomNumber[i]] = currentPeople[i] >= median ? true : false;
    rooms[i].setLight(currentPeople[roomNumber[i]] >= median && rooms[i].peopleCount()>0 ? true : false);
  }
}

void swap(int &fst,int &snd){
  int temp = fst;
  fst = snd;
  snd = temp;
}

//==================================MESSAGE PROCESSING FUNCTIONS==================================



void receiveMsg(){
  static int msgLength = 0;
  char endMarker = '\n';
  char currentChar;

  while (Serial.available() > 0 && !newLine){
    currentChar = Serial.read();
    if (currentChar != endMarker){
      txtBuffer[msgLength] = currentChar;
      msgLength++;
      
      // buffer overflow safety measure
      if (msgLength>=txtBufferSize){
        msgLength = txtBufferSize - 1;
      }
    }else{
      txtBuffer[msgLength] = '\0';
      txtLength = msgLength;
      msgLength = 0;
      // Serial.println(txtBuffer);
      newLine = true;
      // break;
    }
  }
}

void sendInfo(){
  if (currentMode == sendMode){
    const String ROOM = "LIGHT";
    const String COMMA = ",";
    const char endMarker = '\n';
    String roomTxtData[sizeof(rooms)/sizeof(Room)];

    // first, contruct the line

    for (int i=0;i<sizeof(rooms)/sizeof(Room);i++){
      String roomNumberStr = i > 10 ? String(i+1) : "0"+String(i+1);
      roomTxtData[i] = ROOM + roomNumberStr + COMMA + (rooms[i].light() ? "ON" : "OFF");
      if (i<(sizeof(counters)/sizeof(CounterModule))-1) roomTxtData[i] = roomTxtData[i] + COMMA;
      // Serial.println(roomTxtData[i]);
    }

    // Construct msg to send it to the game here
    String gameMsg = "IOT,SEND,";
    for (int i=0;i<sizeof(rooms)/sizeof(Room);i++){
      gameMsg = gameMsg + roomTxtData[i];
    }

    // Serial.println(gameMsg);
    char gameMsgCharArr[gameMsg.length()+1];
    gameMsg.toCharArray(gameMsgCharArr, gameMsg.length()+1);
    Serial.println(gameMsgCharArr);

  }else if(currentMode == confirmMode){
    const char confMsg[]= "Change complete";
    Serial.println(confMsg);
  }
}

void parseAndExecuteMsg(){
  char *stringTokenIndex;

  // first, parse mode
  int newLineLocation = 0;
  int commaCount = 0;
  const char NEW_LINE = '\n';
  const char COMMA = ',';

  for(int i=0;i<txtLength;i++){
    if(txtBuffer[i]==NEW_LINE) break;
    if(txtBuffer[i]==COMMA) commaCount++;
    newLineLocation++;
  }


  stringTokenIndex = strtok(txtBuffer,",");
  stringTokenIndex = strtok(NULL,",");
  currentMode = String(stringTokenIndex);

  // Serial.println(currentMode);
  // then check for the text behind
  int roomNumber = 0;
  if (currentMode == pullMode){
    if (commaCount>1) {
      int roomNumberArray[sizeof(rooms)/sizeof(Room)];
      int roomNumberCount = 0;
      

      for(int i=1;i<commaCount;i++){
        char currentText[txtBufferSize];
        stringTokenIndex = strtok(NULL,",");
        strcpy(currentText,stringTokenIndex);

        // Serial.println(currentText);

        for(int j=0;j<sizeof(currentText)/sizeof(char);j++){
          if (currentText[j] == '\0'){
            const char roomNumberTemp[3] = {currentText[j-2],currentText[j-1],'\0'};
            // Serial.println(roomNumberTemp);

            roomNumberArray[roomNumberCount] = atoi(roomNumberTemp);
            // Serial.println(roomNumber);
            roomNumberCount++;
            break;
          }
        }

      }

      const String ROOM = "LIGHT";
      const String COMMA = ",";
      const char endMarker = '\n';
      String roomTxtData[sizeof(counters)/sizeof(CounterModule)];

      // first, contruct the line
      for (int i=0;i<roomNumberCount;i++){
        String roomNumberStr = roomNumberArray[i] > 10 ? roomNumberArray[i] : "0"+String(roomNumberArray[i]);
        roomTxtData[i] = ROOM + roomNumberStr + COMMA + (rooms[i].light() ? "ON" : "OFF");
        
        if (i!=roomNumberCount-1) roomTxtData[i] = roomTxtData[i] + COMMA;
        // Serial.println(roomTxtData[i]);
        // delay(1000);
      }


      // Construct msg to send it to the game here
      String gameMsg = "IOT,SEND,";
      for (int i=0;i<roomNumberCount;i++){
        gameMsg = gameMsg + roomTxtData[i];
      }

      // Serial.println(gameMsg);
      char gameMsgCharArr[gameMsg.length()+1];
      gameMsg.toCharArray(gameMsgCharArr, gameMsg.length()+1);
      Serial.println(gameMsgCharArr);

    }else {
      // Serial.println("Standard Pull");
      currentMode = sendMode;
      sendInfo();
    }
  }else if (currentMode == changeMode){
    // check which room has to get its light status changed
    int roomNumber = 0;

    
    for(int i=1;i<commaCount;i++){
      char currentText[txtBufferSize];
      stringTokenIndex = strtok(NULL,",");
      strcpy(currentText,stringTokenIndex);

      if(i%2==1){
        //check for room number
        for(int j=0;j<sizeof(currentText)/sizeof(char);j++){
          if (currentText[j] == '\0'){
            char roomNumberTemp[3] = {currentText[j-2],currentText[j-1],'\0'};
            // Serial.println(roomNumberTemp);
            roomNumber = atoi(roomNumberTemp)-1;
            
            // Serial.println(roomNumber);
            break;
          }
        }

      }else{
        // Serial.println("LIGHT0" + String(roomNumber+1) + (rooms[roomNumber].light() ? "ON" : "OFF"));
        rooms[roomNumber].setLight(String(currentText) == "ON" ? true : false);
        // Serial.println("Now: LIGHT0" + String(roomNumber+1) + (rooms[roomNumber].light() ? "ON" : "OFF"));
        // delay(1000);
      }
    }


    // then start the timer
    timer = millis();
    timerStartAt = millis();
    timerEndsAt = timerStartAt + changeCooldown;
    changeCooldownTimerStart = true;

    currentMode = confirmMode;
    sendInfo();
  }
  

  newLine = false;
}



//===============================================================================================