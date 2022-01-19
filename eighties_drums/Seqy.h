/**
 * Seqy - a class to sequences
 * 18 Jan 2022 - Tod Kurt @todbot
 * See https://github.com/todbot/mozzi_experiments/ for examples
 */
class Seqy
{
  public:
  
  Seqy(): enabled(false), seq_id(0), pos(0)
  {
  }

  // which pattern to play  
  void setSeqId(uint8_t id) { seq_id = id % seq_count; }a
  uint8_t getSeqId() { return seq_id; }
  uint8_t getSeqCount() { return seq_count; }
  
  // set the function to call at the top of a beat
  void setBeatHandler(void (*aBeatHandler)(uint8_t beatnum)) {
    beatHandler = aBeatHandler;
  }
  
  // set the function to call to trigger drum samples
  void setTriggerHandler(void (*aTriggerHandler)(bool bd, bool sd,bool ch, bool oh)) {
    triggerHandler = aTriggerHandler;
  }
  
  //   
  void setBPM( float bpm ) {
    bpm = bpm;
    per_beat_millis = 1000 * 60 / bpm / 4;
  }
  
  void update() {
    uint32_t now = millis();
    if( now - last_beat_millis < per_beat_millis ) { return; }
    
    last_beat_millis = now;

    if( beatHandler ) { beatHandler( pos ); }
    
    uint16_t* seq = seqs[ seq_id ];
    uint16_t seq_bd = seq[0];
    uint16_t seq_sd = seq[1];
    uint16_t seq_ch = seq[2];
    uint16_t seq_oh = seq[3];

    // pos is current position in sequence
    pos = (pos+1) % seq_len; // seq_len is int16_t
   
    bool bd_on = seq_bd & (1<<pos); // 0 if none, non-zero if trig
    bool sd_on = seq_sd & (1<<pos); // 0 if none, non-zero if trig
    bool ch_on = seq_ch & (1<<pos); // 0 if none, non-zero if trig
    bool oh_on = seq_oh & (1<<pos); // 0 if none, non-zero if trig

    if( triggerHandler ) { triggerHandler(bd_on, sd_on, ch_on, oh_on ); }
  }

  private:
    bool    enabled;       // is seq playing or not
    float   bpm;           // our arps per minute
    uint8_t foop;
    uint8_t seq_id;        // which arp we using
    uint8_t pos;           // where in current seq we are
    uint16_t per_beat_millis; // = 1000 * 60 / bpm;
    uint32_t last_beat_millis;
    
    void (*triggerHandler)(bool bd, bool sd, bool ch, bool oh) = nullptr;
    void (*beatHandler)(uint8_t beat_num) = nullptr;

    static const int seq_len = 16;  // number of trigs in an seq
    static const int seq_inst_num = 4;
    static const uint8_t seq_count= 3;   // how many seqs in "seqs"
    
    uint16_t seqs[seq_count][seq_inst_num] = {
      {
        0b1000000010000001, // bd
        0b0000100000001000, // sd
        0b1010101010101010, // ch
        0b0000000000000001, // oh
      },
      {
        0b1000000010000001, // bd
        0b0000100000001000, // sd
        0b1111111011111110, // ch
        0b0000000100000001, // oh
      },
      {
        0b1100110011001001, // bd
        0b0000100000001000, // sd
        0b0011001100110011, // ch
        0b1000000110000001, // oh
      },
    };
   
};
