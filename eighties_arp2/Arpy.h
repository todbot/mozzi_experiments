
class Arpy 
{
  private:
    bool    enabled;       // is arp playing or not
    float   bpm;           // our arps per minute
    uint8_t arp_id;        // which arp we using
    uint8_t arp_pos;       // where in current arp are we
    uint8_t tr_steps;      // if transposing, how many times, 1= normal
    int8_t  tr_dist;       // like an octave
    uint8_t tr_pos;        // which tranposed we're on (0..tr_steps)
    uint8_t root_note;     // 
    uint16_t per_beat_millis; // = 1000 * 60 / bpm;

    uint8_t note_played;    // the note we have played, so we can unplay it later
    uint16_t note_duration; // = 0.5 * per_beat_millis; // 50% gate
    uint32_t last_beat_millis; 

    void (*noteOnHandler)(uint8_t) = nullptr;
    void (*noteOffHandler)(uint8_t) = nullptr;

    const int arp_len = 4;  // number of notes in an arp 
    const uint8_t arp_count= 7;   // how many arps in "arps"
    int8_t arps[7][4] = { 
      {0, 4, 7, 12},    // major
      {0, 3, 7, 10},    // minor 7th 
      {0, 7, 12,0},     // 7 Power 5th
      {0, 12, 0, -12},  // octaves
      {0, 12, 24, -12}, // octaves 2
      {0, -12, -12, 0}, // octaves 3 (bass)
      {0, 0, 0, 0},     // root
    };


  public:
  
  Arpy(): enabled(false), arp_id(0), arp_pos(0), root_note(0), note_played(0)
  {
    tr_steps = 1;
    tr_dist = 12;
    tr_pos = 0;
  }
  void on() { enabled = true; }
  void off() { enabled = false; }

  void setRootNote( uint8_t note ) { root_note = note; }
  
  void setNoteOnHandler(void (*anoteOnHandler)(uint8_t)) {
    noteOnHandler = anoteOnHandler;
  }
  void setNoteOffHandler(void (*anoteOffHandler)(uint8_t)) {
    noteOffHandler = anoteOffHandler;
  }

  void setBPM( float bpm ) { 
    bpm = bpm;
    per_beat_millis = 1000 * 60 / bpm;
    note_duration = 0.5 * per_beat_millis; // 50% gate  FIXME: make 0.5 a param
  }

  void setTransposeSteps(uint8_t steps) {  tr_steps = steps; }
  void setTransposeDistance(uint8_t dist) {  tr_dist = dist; }
    
  void setArp(uint8_t arp_id) {
    arp_id = arp_id % arp_count;    
  }
  void nextArp() {
    arp_id = (arp_id+1) % arp_count;    
  }
  
  void update(uint32_t now, uint8_t root_note_new)
  {
    if( !enabled ) { return;  }
    
    if( now - last_beat_millis > per_beat_millis )  {
      last_beat_millis = now;
      int8_t tr_amount = tr_dist * tr_pos;  // tr_pos may be zero
      
      // only make musical changes at start of a new measure // FIXME allow immediate
      if( arp_pos == 0 ) {
        root_note = root_note_new;
        tr_pos = (tr_pos + 1) % tr_steps;
      }
      note_played = root_note + arps[arp_id][arp_pos] + tr_amount;
      if(noteOnHandler) { noteOnHandler( note_played ); }
      arp_pos = (arp_pos+1) % arp_len;
    }
  
    if( now - last_beat_millis > note_duration ) { 
      if( note_played != 0 ) { // we have a note to turn off!
        if(noteOffHandler) { noteOffHandler( note_played ); }
        note_played = 0;  // say we've turned off the note
      }
    }
  }

};
