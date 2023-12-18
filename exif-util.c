#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "exif-util.h"

struct jpg_time *get_jpg_time(char* old_location) {
        struct jpg_time *res = (struct jpg_time*) malloc(sizeof(struct jpg_time));
        // struct jpg_time *res;

        // This code is non-trivial. I did not want to include an Exif library just to read a simple field.
        FILE *stream = fopen(old_location, "r+");
        // First thing, cheick if it is an Exif formatted JPG. If not, abandon ship.
        // We already know that the file is JPG, so the first 2 bytes are fluff.
        fseek(stream, 0x02, SEEK_SET);
        // I am going to walk an arbitrary number of bytes looking for the Exif marker.
        // It would probably be safe to assume if we walk over a hundred bytes into the file,
        // it is not encoded with EXIF.
        bool is_exif = false;
        unsigned char encoding_type;
        for (int i = 0; i < 100; i++) {
            // read_n_bytes(encoding_type, 1,stream);
	    encoding_type = fgetc(stream);
            // printf("bytes: %x\n", encoding_type);
            if (encoding_type == 0xE1) {
                is_exif = true;
                break;
            }
        }
        if (is_exif) printf("IMAGE IS EXIF FORMAT!\n");
        if (!is_exif) return NULL;
        
        // Might be an error if 0xE1 was not the end of the sequence.
        // next 8 bytes unneeded
        fseek(stream, 0x08, SEEK_CUR);
        uint32_t main_ofs = ftell(stream);
        bool big_endian = false;
        // Thank you Johnathan Swift
        unsigned char endianness[1];
        read_n_bytes(endianness, 2, stream);
        
        if (endianness[0] == 0x4D && endianness[1] == 0x4D) {
            big_endian = true;
        }
        // If we are using Intel encoding, we need to read in the buffer backwards.
        fseek(stream, 0x02, SEEK_CUR);
        // Following 2 bytes unneeded, then the next 4 determine the offset into IFD directory
        // structure relative to the position of the endianness marker.
        unsigned char ifd_offset_bytes[4];
        read_n_bytes(ifd_offset_bytes, 4, stream);
        uint32_t ifs_offs = 0;
        if (!big_endian) {
            for (int i = 0; i <= 3; i++) {
                ifs_offs += (uint32_t) ifd_offset_bytes[i];
            }
        } else {
            for (int i = 3; i >= 0; i--) {
                ifs_offs += (uint32_t) ifd_offset_bytes[i];
            }
        }
        // Now we want to seek to the position of the tags!
        // Note that the first two bytes indicate the number of tags, but we're going 
        // to disregard that because the algorithm is simply searching for the 
        // DateTime tag. We may want to improve the logic by making sure we are within
        // the bounds of the Exif tag, but for now we'll just walk an arbitrary length.
        fseek(stream, (main_ofs + ifs_offs + 2) - ftell(stream), SEEK_CUR);
        // fseek(stream, (main_ofs + ifs_offs) - ftell(stream), SEEK_CUR);

        int walked = 0;
        unsigned char tag_id[1];
        // unsigned char datetime_tag = 0x0132;
        unsigned char date_a_1, date_a_2, date_b_1, date_b_2;
        if (big_endian) {
            // I am accounting for the DateTimeOriginal and ModifyDate tags, but
            // depending on the camera firmware there's probably many I'm missing. 
            date_a_1 = 0x01;
            date_a_2 = 0x32;
            date_b_1 = 0x90;
            date_b_2 = 0x03;
        } else {
            date_a_1 = 0x32;
            date_a_2 = 0x01;
            date_b_1 = 0x03;
            date_b_2 = 0x90;
        }


        // todo: replace by looking at how many IFD entries there are.
        while (walked < 6000) {
            read_n_bytes(tag_id, 2, stream);
            printf("%x\n", tag_id[0]);
            printf("%x\n", tag_id[1]);

            if ((tag_id[0] == date_a_1 && tag_id[1] == date_a_2) || 
                (tag_id[0] == date_b_1 && tag_id[1] == date_b_2)) {
                printf("found match!\n");
                break;
            } else if (tag_id[0] == 0x87 && tag_id[1] == 0x69 || tag_id[0] == 0x69 && 
                        tag_id[1] == 0x87) {
                // This tag means that the first IFD has ended and we need to seek to the next offset.
                // I believe the 6 bytes following the tag indicate the offset from the initial endianness
                // indicator, but I could be *very* wrong.
                fseek(stream, 6, SEEK_CUR);
                char next_ifd_offset[2];
                read_n_bytes(next_ifd_offset, 2, stream);
                uint32_t next_ofs = 0x00000000;
                if (!big_endian) {
                    for (int i = 0; i < 2; i++) {
                        // math
                        next_ofs |= (uint32_t) next_ifd_offset[i] << (i * 2) * 4;
                    }
                } else {
                    for (int i = 3; i >= 0; i--) {
                        next_ofs += (uint32_t) next_ifd_offset[i];
                    }
                }
                fseek(stream, (next_ofs + main_ofs + 2) - ftell(stream), SEEK_CUR);
                // After this, the IFD can be interpreted the same.
                next_ofs = 0;
                continue;
                }
            fseek(stream, 10, SEEK_CUR);

            walked += 12;
            // I already accounted for endinaess when I prepared the tag reference.
        }
        if (walked > 5900) exit(1);
        // It looks like we found the DateTime field.
        // Now, we walk 6 bytes into that field to read the location of the date.
        fseek(stream, 6, SEEK_CUR);
        unsigned char date_ptr[3];
        read_n_bytes(date_ptr, 4, stream);
        uint32_t datefield_ofs = 0x00000000;
        if (!big_endian) {
            for (int i = 0; i <= 3; i++) {
                // math
                datefield_ofs |= (uint32_t) date_ptr[i] << (i * 2) * 4;
            }
        } else {
            for (int i = 3; i >= 0; i--) {
                datefield_ofs += (uint32_t) date_ptr[i];
            }
        }
        fseek(stream, (datefield_ofs + main_ofs) - ftell(stream), SEEK_CUR);
        // The Exif encoding of the date should be as follows:
        // <year>:<month, 2 digits>:<day, 2 digits> <ascii space> <time>
        // It is far easier to just read from disk instead of dealing with the data in memory.
        // Plus, the file system reads 4k chunks from each file, and the metadata is typically
        // less than that. So realistically, all of this should require only 1 disk read on average.

        read_n_bytes(res->year_s, 4, stream);
        res->year_s[4] = '\0';
        fseek(stream, 1, SEEK_CUR);
        read_n_bytes(res->month_s, 2, stream);
        res->month_s[2] = '\0';
        fseek(stream, 1, SEEK_CUR);

        read_n_bytes(res->day_s, 2, stream);
        res->day_s[2] = '\0';
        fseek(stream, 1, SEEK_CUR);

        read_n_bytes(res->time_s, 8, stream);
        res->time_s[8] = '\0';
        if (!res->year_s) return NULL;
        fclose(stream);
        return res;
}