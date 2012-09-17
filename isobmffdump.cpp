// mandatory includes
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <algorithm>

// defines and typedefs
#define  SW16(v)     ((((uint16_t)(v) <<  8) & 0xff00) | \
                      (((uint16_t)(v) >>  8) & 0x00ff))
#define  SW32(v)     ((((uint32_t)(v) << 24) & 0xff000000) | \
                      (((uint32_t)(v) <<  8) & 0x00ff0000) | \
                      (((uint32_t)(v) >>  8) & 0x0000ff00) | \
                      (((uint32_t)(v) >> 24) & 0x000000ff))
#define  SW64(v)     ((((uint64_t)(v) << 56) & 0xff00000000000000) | \
                      (((uint64_t)(v) << 40) & 0x00ff000000000000) | \
                      (((uint64_t)(v) << 24) & 0x0000ff0000000000) | \
                      (((uint64_t)(v) <<  8) & 0x000000ff00000000) | \
                      (((uint64_t)(v) >>  8) & 0x00000000ff000000) | \
                      (((uint64_t)(v) >> 24) & 0x0000000000ff0000) | \
                      (((uint64_t)(v) >> 40) & 0x000000000000ff00) | \
                      (((uint64_t)(v) >> 56) & 0x00000000000000ff))
#define    R8(o)                       (*(data + data_offset + (o)))
#define   R16(o)     SW16(*((uint16_t *)(data + data_offset + (o))))
#define   R32(o)     SW32(*((uint32_t *)(data + data_offset + (o))))
#define   R64(o)     SW64(*((uint64_t *)(data + data_offset + (o))))

struct ATOM
{
    uint32_t type;
    uint32_t size;
} atoms[] =
{   // isom/iso2 atoms (ISO 14496-12 #6.2.3)
    { 0x766f6f6d,   0 }, // moov
    { 0x6b617274,   0 }, // trak
    { 0x73746465,   0 }, // edts
    { 0x6169646d,   0 }, // mdia
    { 0x666e696d,   0 }, // minf
    { 0x666e6964,   0 }, // dinf
    { 0x6c627473,   0 }, // stbl
    { 0x64737473,   8 }, // stsd
    { 0x7865766d,   0 }, // mvex
    { 0x61746475,   0 }, // udta
    { 0x666f6f6d,   0 }, // moof
    { 0x66617274,   0 }, // traf
    { 0x6172666d,   0 }, // mfra
    { 0x6f727069,   0 }, // ipro
    { 0x666e6973,   0 }, // sinf
    { 0x69686373,   0 }, // schi
    { 0x6e696966,   0 }, // fiin
    { 0x6e656170,   0 }, // paen
    { 0x6f63656d,   0 }, // meco

    // aac/avc1 extensions atoms
    { 0x61636e65,  28 }, // enca
    { 0x6134706d,  28 }, // mp4a
    { 0x76636e65,  78 }, // encv
    { 0x31637661,  78 }, // avc1
    { 0x32637661,  78 }, // avc2
    { 0x7634706d,  78 }, // mp4v

    // end marker
    { 0x00000000,   0 }
};

// decode single atom
bool atom_decode(u_char *data, size_t offset, size_t total, size_t *size, uint32_t *type, uint32_t *header_size)
{
    if (total - offset < 8)
    {
        return false;
    }
    if (header_size)
    {
        *header_size = 8;
    }
    *size = ((size_t)data[offset] << 24) + ((size_t)data[offset + 1] << 16) + ((size_t)data[offset + 2] << 8) + (size_t)data[offset + 3];
    if (*size == 0)
    {
        *size = total - offset;
    }
    else if (*size == 1)
    {
        if (total - offset < 16)
        {
            return false;
        }
        if (header_size)
        {
            *header_size = 16;
        }
        *size = ((size_t)data[offset + 8]  << 56) + ((size_t)data[offset + 9]  << 48) + ((size_t)data[offset + 10] << 40) + ((size_t)data[offset + 11] << 32) +
                ((size_t)data[offset + 12] << 24) + ((size_t)data[offset + 13] << 16) + ((size_t)data[offset + 14] <<  8) + ((size_t)data[offset + 15]);
    }
    if (*size < 8)
    {
        return false;
    }
    memcpy(type, data + offset + 4, 4);
    return true;
}

// return atom name
char *atom_name(uint32_t type)
{
    static char name[5];
    int         index;

    for (index = 0; index < 4; index ++)
    {
       name[index] = (type >> (24 - (8 * (3 - index)))) & 0xff;
    }
    name[4] = 0;
    return name;
}

// human-readable memory dump
void dump(u_char *input, size_t size, size_t indent, bool raw)
{
    u_int32_t index1 = 0, index2 = 0;
    char      ascii[64], spaces[128];

    memset(spaces, ' ', sizeof(spaces));
    spaces[std::min(indent, sizeof(spaces) - 1)] = 0;
    memset(ascii, 0, sizeof(ascii));
    while (index1 < size)
    {
        if (raw)
        {
            printf(isprint(input[index1]) || isspace(input[index1]) ? "%c" : "\\x%02x", input[index1]);
        }
        else
        {
            if (!(index1 % 32))
            {
                printf("%s%s%s%s%08x  ", *ascii ? " " : "", ascii, *ascii ? "\n" : "", spaces, index1);
                memset(ascii, 0, sizeof(ascii));
                index2 = 0;
            }
            else if (index1 && !(index1 % 16))
            {
                printf(" ");
            }
            printf("%02x ", input[index1]);
            ascii[index2 ++] = isprint(input[index1]) ? input[index1] : '.';
        }
        index1 ++;
    }
    if (raw) printf("\n");
    index1 = strlen(ascii);
    if (index1)
    {
        for (index2 = 0; index2 < 32 - index1; index2 ++)
        {
            printf("   ");
        }
        printf("%s %s\n", index1 >= 16 ? "" : " ", ascii);
    }
}

// main program entry
int main(int argc, char **argv)
{
    u_char   *data;
    char     space[BUFSIZ], *dumps[64];
    off_t    data_size, data_offset = 0;
    size_t   atom_size, levels[64], level = 0;
    uint32_t atom_type, header_size;
    int      handle, index, raw_dump = false, dumps_count = 0;

    static struct option options[] =
    {
        {"dump-raw", 0, NULL, 'r'},
        {"dump",     1, NULL, 'd'},
        {NULL,       0, NULL,   0}
    };
    while ((index = getopt_long(argc, argv, "d:r", options, NULL)) != -1)
    {
        switch (index)
        {
            case 'd':
                if (dumps_count < 64) dumps[dumps_count ++] = strdup(optarg);
                break;

            case 'r':
                raw_dump = true;
                break;

            case '?':
                return 1;
                break;
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "usage: isodump [--dump-raw] [--dump <atom>] <file>\n");
        return 1;
    }
    if ((handle = open(argv[optind], O_RDONLY)) < 0 || (data_size = lseek(handle, 0, SEEK_END)) < 0 || (data = (u_char *)mmap(NULL, data_size, PROT_READ, MAP_PRIVATE, handle, 0)) == MAP_FAILED)
    {
        fprintf(stderr, "cannot open %s\n", argv[optind]);
        return 2;
    }
    while (data_offset < data_size)
    {
        if (!atom_decode(data, data_offset, data_size, &atom_size, &atom_type, &header_size))
        {
            break;
        }
        if (level && (data_offset + atom_size) > levels[level - 1])
        {
            data_offset = levels[level - 1];
            level --;
            continue;
        }
        memset(space, ' ', sizeof(space));
        space[std::min(level * 2, sizeof(space) - 1)] = 0;
        atom_size = std::min((off_t)atom_size, data_size - data_offset);
        if (!raw_dump) printf("@%-10ld| %s%s [%ld]\n", data_offset, space, atom_name(atom_type), atom_size);
        for (index = 0; index < dumps_count; index ++)
        {
            if (!memcmp(dumps[index], atom_name(atom_type), 4))
            {
                dump(data + data_offset + 8, atom_size - 8, 13 + (level * 2), raw_dump);
                break;
            }
        }
        index = 0;
        while (atoms[index].type)
        {
            if (atom_type == atoms[index].type)
            {
                levels[level ++] = data_offset + atom_size;
                data_offset += header_size + atoms[index].size;
                break;
            }
            index ++;
        }
        if (!atoms[index].type)
        {
            data_offset += atom_size;
        }
        if ((size_t)data_offset < levels[level - 1] && (levels[level - 1] - data_offset) < 8)
        {
            data_offset = levels[level - 1];
        }
        while (level && (size_t)data_offset >= levels[level - 1])
        {
            level --;
            level = (level < 0 ? 0 : level);
        }
    }
    if (!raw_dump) printf("@%-10ld| end\n", data_offset);
    return 0;
}
