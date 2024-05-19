#include <stdlib.h>
#include <cstdint>

#ifndef Explorer_File_h
#define Explorer_File_h

class ExplorerFile
{
private:

    // identification
	uint64_t file_id;
	char *file_path;
    uint8_t file_type;

    // user added tags
	char *tags;
    uint8_t num_tags;

    // bpm
    uint16_t pred_bpm;
    uint16_t entr_bpm;
	
    // key
	uint8_t  pred_key;
	uint8_t  entr_key;

    // instruments
	uint8_t  pred_instr;
	uint8_t  entr_instr;

};

#endif