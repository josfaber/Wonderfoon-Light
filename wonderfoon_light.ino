/***************************************************
  Wonderfoon Light
  
  @author   - Jos Faber
  @email    - jfaber@protonmail.com

  Deze app speelt de mp3's op de SD kaart random 
  achter elkaar af zodra de hoorn van de haak
  wordt gehaald

  SD kaart indeling:
    mp3/
      0001.mp3
      0002.mp3
      ...
      
  Libraries nodig:
    https://github.com/DFRobot/DFRobotDFPlayerMini
    
 ****************************************************/
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

const int RX_PIN = 11;
const int TX_PIN = 10;
const int BUSY_PIN = 12;
const int HOOK_PIN = 9;

const int DFPLAYER_DELAY = 100; // ms
const int PLAY_DELAY = 1600;    // ms
const int POLL_DELAY = 200;     //ms

const int WF_VOLUME = 24;     // 0 - 30

const int IS_PLAYING = 0;  // low
const int NOT_PLAYING = 1; // high
const int IS_IDLE = 0;     // low
const int IS_ACTIVE = 1;   // high

int wonderfoon_state = IS_IDLE;

int num_tracks;
int tracks[255];
int cur_track_index = -1;
int nextPlayAwaitMillis = 0; // ms
bool nextPlayAwait = false;

// instantiate hardware and serial
SoftwareSerial wfSerial(TX_PIN, RX_PIN);
DFRobotDFPlayerMini wfPlayer;
void handleType(uint8_t type, int value);

/**
 * -----------------------------
 * 
 * Setup
 * -----------------------------
 */
void setup()
{
  pinMode(BUSY_PIN, INPUT);
  pinMode(HOOK_PIN, INPUT_PULLUP);

  wfSerial.begin(9600);
  Serial.begin(115200);

  if (!wfPlayer.begin(wfSerial))
  {
    Serial.println(F("Fout van DFPlayer"));
    Serial.println(F("1. Kijk aansluitingen na (RX en TX misschien verkeerd om"));
    Serial.println(F("2. Kijk SD kaart na"));
    while (true)
      ;
  }
  Serial.println(F("DFPlayer online!"));

  // Volume
  wfPlayer.volume(WF_VOLUME);
  delay(DFPLAYER_DELAY);

  // Lees aantal tracks
  num_tracks = wfPlayer.readFileCounts();
  createRandomTracks();
  delay(DFPLAYER_DELAY);
}

/**
 * -----------------------------
 * 
 * Loop
 * -----------------------------
 */
void loop()
{
  static unsigned long timer = millis();

  // Lees om de [n] ms de pins
  if (millis() - timer > POLL_DELAY)
  {
    timer = millis();

    int hook_state = digitalRead(HOOK_PIN);

    // Haak wordt opgepakt
    if (hook_state == IS_ACTIVE && wonderfoon_state == IS_IDLE)
    {
      Serial.println(F("wonderfoon_state from IS_IDLE -> IS_ACTIVE"));
      wonderfoon_state = IS_ACTIVE;

      // Begin met spelen
      delay(PLAY_DELAY);
      nextTrack();
    }

    // Haak wordt opgelegd
    else if (hook_state == IS_IDLE && wonderfoon_state == IS_ACTIVE)
    {
      Serial.println(F("wonderfoon_state from IS_ACTIVE -> IS_IDLE"));

      // Set state
      wonderfoon_state = IS_IDLE;

      // Stop met spelen
      wfPlayer.stop();

      // Reset
      nextPlayAwait = false;

      // Shuffle tracks voor volgende keer
      createRandomTracks();
    }

    // Alleen als Wonderfoon actief
    if (wonderfoon_state == IS_ACTIVE)
    {
      int busy_state = digitalRead(BUSY_PIN);
      if (busy_state == NOT_PLAYING)
      {
        if (nextPlayAwait == false)
        {
          Serial.println(F("Queue next play"));
          nextPlayAwait = true;
          nextPlayAwaitMillis = timer;
        }
        else if (timer - nextPlayAwaitMillis > PLAY_DELAY)
        {
          Serial.println(F("PLAY_DELAY reached, playing next"));
          nextPlayAwait = false;
          nextPlayAwaitMillis = 0;
          nextTrack();
        }
      }
    }
  }
}

/**
 * -----------------------------
 * 
 * Create array with range
 * and shuffle
 * -----------------------------
 */
void createRandomTracks()
{
  //  Serial.println(F("createRandomTracks"));

  for (byte i = 1; i <= num_tracks; i = i + 1)
  {
    tracks[i - 1] = i;
  }

  shuffleArray(tracks, num_tracks);
}

/**
 * -----------------------------
 * 
 * Play the track of next
 * index from the array
 * -----------------------------
 */
void nextTrack()
{
  if (wonderfoon_state == IS_ACTIVE)
  {
    if (cur_track_index < num_tracks - 1)
    {
      cur_track_index = cur_track_index + 1;
    }
    else
    {
      cur_track_index = 0;
    }
    Serial.print(F("Next track, playing index "));
    Serial.print(cur_track_index);
    Serial.print(F(": track "));
    Serial.println(tracks[cur_track_index]);

    wfPlayer.play(tracks[cur_track_index]);
    delay(DFPLAYER_DELAY);
  }
}

/**
 * -----------------------------
 * 
 * Shuffle an array based 
 * on size
 * -----------------------------
 */
void shuffleArray(int *array, int size)
{
  //  Serial.println(F("shuffleArray"));

  randomSeed(analogRead(A0)); // real randomness
  int last = 0;
  int temp = array[last];
  for (int i = 0; i < size; i++)
  {
    int index = random(size);
    array[last] = array[index];
    last = index;
  }
  array[last] = temp;
}
