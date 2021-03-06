#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utils.h"
#include "mdazibao.h"
#include "alternate_cmd.h"

/** @file */

/** buffer size used in various functions */
#define BUFFSIZE 512

int check_option_add(int argc, char **argv, int *date_idx, int *compound_idx,
                int *dz_idx, int *type_idx, int *input_idx) {
        int idx = 0,
            args_count = 0;
        for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "--type")) {
                        *type_idx = idx;
                } else if ((strcmp(argv[i], "--date") == 0)) {
                        if (*date_idx < 0) {
                                *date_idx = idx;
                        }
                } else if (strcmp(argv[i], "--dazibao") == 0) {
                        if (*dz_idx < 0) {
                                *dz_idx = idx;
                        }
                } else if (strcmp(argv[i], "--compound") == 0) {
                        if (*compound_idx < 0) {
                                *compound_idx = idx;
                        }
                } else if (strcmp(argv[i], "-") == 0) {
                        if (*input_idx < 0) {
                                *input_idx = idx;
                                argv[idx] = argv[i];
                                idx++;
                                args_count++;
                        }
                } else {
                        /* if the current parameter is not an option */
                        argv[idx] = argv[i];
                        idx++;
                        args_count++;
                }
        }
        return args_count;
}

int check_type_args(int argc, char *type_args, char *op_type, int f_dz) {
        char * delim = ",";
        char *tmp = strtok(op_type, delim);
        int i = 0;
        while (tmp != NULL) {
                int tmp_type = tlv_str2type(tmp);
                if (tmp_type == (char) -1) {
                        printf("unrecognized type %s\n", tmp);
                        return -1;
                } else {
                        type_args[i] = tmp_type;
                }
                tmp = strtok(NULL, delim);
                i++;
        }

        if (i != (argc + (f_dz >= 0 ? -1 : 0))) {
                printf("args to option type too large\n");
                return -1;
        }
        return 0;
}

int check_args(int argc, char **argv, int *f_dz, int *f_co, int *f_d) {
        int date_size = 0,
            compound_size = 0,
            tmp_size = 0,
            i;
        for (i = 0; i < argc; i++) {
                if (strcmp(argv[i],"-") == 0) {
                        continue;
                } else if ( i == *f_dz) {
                        if ((tmp_size = check_dz_path(argv[i], R_OK)) < 0) {
                                printf("check dz arg failed :%s\n", argv[i]);
                                return -1;
                        }
                } else if ( i >= *f_co) {
                        if ((tmp_size = check_tlv_path(argv[i], R_OK)) < 0) {
                                printf("check path arg failed :%s\n", argv[i]);
                                return -1;
                        }
                        compound_size += tmp_size;
                } else {
                        tmp_size = check_tlv_path(argv[i], R_OK);
                        if (tmp_size < 0) {
                                printf("check arg failed :%s\n", argv[i]);
                                return -1;
                        }
                }
                /* check size file for tlv */
                if ((*f_d <= *f_co) && (i >= *f_co)) {
                        date_size += compound_size +
                                TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                } else {
                        date_size += tmp_size;
                }

                if (tmp_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv too large, %s\n",argv[i]);
                        return -1;
                } else if (date_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv date too large, %s\n",argv[i]);
                        return -1;
                } else if (compound_size > TLV_MAX_VALUE_SIZE) {
                        printf("tlv compound too large, %s\n",argv[i]);
                        return -1;
                }
                tmp_size = 0;
                date_size = 0;
        }
        return 0;
}

int cmd_add(int argc, char **argv, char * daz) {
        int f_date = -1,
            f_compound = -1,
            f_dz = -1,
            f_type = -1,
            f_input = -1;
        char *type_args = NULL;

        if (argc < 0) {
                printf("error no args for add\n");
                return -1;
        }

        argc = check_option_add(argc, argv, &f_date, &f_compound, &f_dz,
                        &f_type, &f_input);

        if (f_type >= 0) {
                type_args = (char *) malloc(sizeof(*type_args)* argc);
                char *tmp = argv[f_type];
                if (f_type < (argc -1)) {
                        /* deleted type args in argv*/
                        int i;
                        for (i = (f_type + 1); i < (argc); i++) {
                                argv[i-1] = argv[i];
                        }
                        /* shift flag if it after type args */
                        f_date  = (f_date > f_type ? f_date -1 : f_date);
                        f_compound = (f_compound > f_type ? f_compound-1 :
                                        f_compound);
                        f_input = (f_input > f_type ? f_input-1 : f_input);
                        f_dz = (f_dz > f_type ? f_dz-1 : f_dz);
                }
                argc--;
                if (check_type_args(argc, type_args, tmp, f_dz) < 0) {
                        printf("check_args_no_op failed\n");
                        free(type_args);
                        return -1;
                }
        } else if ( argc > (f_dz >= 0 ? 1:0) + (f_input >= 0 ? 1:0)) {
                printf("check_type_args failed\n");
                return -1;
        }

        if (check_args(argc, argv, &f_dz, &f_compound, &f_date)) {
                printf("check path args failed\n");
                return -1;
        }

        if (action_add(argc, argv, f_compound, f_dz, f_date, f_input,
                type_args, daz) == -1) {
                printf("error action add\n");
                return -1;
        }
        if (f_type >= 0) {
                free(type_args);
        }
        return 0;
}

int action_add(int argc, char **argv, int f_co, int f_dz, int f_d, int f_in,
                char *type, char *daz) {
        dz_t daz_buf;
        unsigned int buff_size_co = 0;
        tlv_t tlv = NULL;
        tlv_t buff_co = NULL;
        tlv_t buff_d = NULL;
        int i,j = 0;

        if (dz_open(&daz_buf, daz, O_RDWR) < 0) {
                fprintf(stderr, "failed open the dazibao\n");
                return -1;
        }

        f_d = (f_d == -1 ? argc : f_d);
        f_co = (f_co == -1 ? argc : f_co);

        for (i = 0; i < argc; i++) {
                int tlv_size = 0;
                /* inizialized tlv */
                if (tlv_init(&tlv) < 0) {
                      printf("error to init tlv\n");
                        return -1;
                }

                /* different possibility to create a standard tlv*/
                if (i == f_in) {
                        tlv_size = tlv_create_input(&tlv, &type[j]);
                        j++;
                } else if (i == f_dz) {
                        tlv_size = dz2tlv(argv[i], &tlv);
                } else {
                        tlv_size = tlv_create_path(argv[i], &tlv, &type[j]);
                        j++;
                }

                /* if not tlv as create error */
                if (tlv_size < 0) {
                        printf("error to create tlv with path %s\n", argv[i]);
                        return -1;
                }

                /* other option who use tlv created */
                if ( i >= f_co ) {
                        /* if tlv to insert to compound it type dated*/
                        if ((i >= f_d) && (f_d > f_co)) {
                                if (tlv_init(&buff_d) < 0) {
                                        printf("error to init tlv compound");
                                        return -1;
                                }
                                tlv_size = tlv_create_date(&buff_d, &tlv,
                                                tlv_size);
                                if (tlv_size < 0) {
                                        printf("error to create tlv dated"
                                                        "%s\n", argv[i]);
                                        return -1;
                                }
                                tlv_destroy(&tlv);
                                tlv = buff_d;
                                buff_d = NULL;
                        }
                        unsigned int size_realloc = TLV_SIZEOF_HEADER;
                        if ((f_co == i) && (tlv_init(&buff_co) < 0)) {
                                printf("error to init tlv compound\n");
                                return -1;
                        }
                        size_realloc += buff_size_co + tlv_size;
                        buff_co = (tlv_t) safe_realloc(buff_co,
                                        sizeof(*buff_co) * size_realloc);
                        if (buff_co == NULL) {
                                ERROR("realloc", -1);
                        }

                        memcpy(buff_co + buff_size_co, tlv, tlv_size);
                        buff_size_co += tlv_size;
                        tlv_destroy(&tlv);

                        /*when all tlv is insert, create tlv compound*/
                        if (i == argc -1) {
                                if (tlv_init(&tlv) < 0) {
                                        printf(" error to init tlv");
                                        return -1;
                                }
                                tlv_size = tlv_create_compound(&tlv, &buff_co,
                                        buff_size_co);
                                if (tlv_size < 0) {
                                        printf(" error to create compound"
                                        " %s\n", argv[i]);
                                        return -1;
                                }
                                tlv_destroy(&buff_co);
                        } else {
                                continue;
                        }
                }

                if ((i >= f_d) && (f_d <= f_co)) {
                        if (tlv_init(&buff_d) < 0) {
                                printf(" error to init tlv dated\n");
                                return -1;
                        }
                        tlv_size = tlv_create_date(&buff_d, &tlv, tlv_size);
                        if (tlv_size < 0) {
                                printf(" error to create tlv dated"
                                        "%s\n", argv[i]);
                                return -1;
                        }
                        tlv_destroy(&tlv);
                        tlv = buff_d;
                        buff_d = NULL;
                }

                if (tlv_size > 0) {
                        if (dz_add_tlv(&daz_buf, &tlv) == -1) {
                                fprintf(stderr, "failed adding the tlv\n");
                                tlv_destroy(&tlv);
                                return -1;
                        }
                        tlv_destroy(&tlv);
                }
        }

        if (dz_close(&daz_buf) < 0) {
                fprintf(stderr, "failed closing the dazibao\n");
                return -1;
        }
        return 0;
}

int choose_tlv_extract(dz_t *dz, tlv_t *tlv, long off) {

        if (dz_tlv_at(dz, tlv, off) < 0) {
                printf("error to read type and length tlv\n");
                dz_close(dz);
                tlv_destroy(tlv);
                return -1;
        }

        if (tlv_get_type(tlv) == TLV_DATED) {
                off = off + TLV_SIZEOF_HEADER + TLV_SIZEOF_DATE;
                return choose_tlv_extract(dz,tlv,off);
        }

        if (dz_read_tlv(dz, tlv, off) < 0) {
                printf("error to read value tlv\n");
                dz_close(dz);
                tlv_destroy(tlv);
                return -1;
        }
        return 0;
}

int cmd_extract(int argc , char **argv, char *dz_path) {
        dz_t dz;
        tlv_t tlv;
        long off;
        int fd;
        size_t real_size;
        char *data;
        char *path;

        if (argc != 2) {
                fprintf(stderr, "cmd extract : <offset> <path> <dazibao>\n");
                return DZ_ARGS_ERROR;
        }

        /* If the offset doesn't start with a character between '0' and '9', it
         * must be wrong.
         */
        if (argv[0][0] < 48 || argv[0][0] > 57) {
                fprintf(stderr,
                        "Usage:\n       extract <offset> <file> <dazibao>\n");
                return DZ_ARGS_ERROR;
        }

        off = str2dec_positive(argv[0]);

        if (off < DAZIBAO_HEADER_SIZE) {
                fprintf(stderr, "wrong offset\n");
                return DZ_OFFSET_ERROR;
        }

        if (dz_open(&dz, dz_path, O_RDWR) < 0) {
                fprintf(stderr, "Error while opening the dazibao\n");
                return -1;
        }

        if (dz_check_tlv_at(&dz, off, -1,NULL) <= 0) {
                fprintf(stderr, "no such TLV\n");
                dz_close(&dz);
                return DZ_OFFSET_ERROR;
        }

        if (tlv_init(&tlv) < 0) {
                printf("error to init tlv\n");
                dz_close(&dz);
                return -1;
        }

        if (choose_tlv_extract(&dz,&tlv,off) < 0) {
                printf("error to init tlv\n");
                tlv_destroy(&tlv);
                dz_close(&dz);
                return -1;
        }

        if (dz_close(&dz) < 0) {
                fprintf(stderr, "Error while closing the dazibao\n");
                tlv_destroy(&tlv);
                return -1;
        }

        if (tlv_get_type(&tlv) == TLV_COMPOUND) {
                real_size = (size_t)tlv_get_length(&tlv) + DAZIBAO_HEADER_SIZE;
        } else {
                real_size = (size_t)tlv_get_length(&tlv);
        }

        path = argv[1];
        fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);

        if (fd == -1) {
                tlv_destroy(&tlv);
                ERROR("open", -1);
        }

        if (ftruncate(fd, real_size) < 0) {
                fprintf(stderr, "Error while ftruncate path");
                close(fd);
                return -1;
        }

        data = (char*)mmap(NULL, real_size, PROT_WRITE, MAP_SHARED, fd, 0);

        if (data == MAP_FAILED) {
                fprintf(stderr, "Error while mmap ");
                close(fd);
                tlv_destroy(&tlv);
                return -1;
        }

        if (tlv_get_type(&tlv) == TLV_COMPOUND) {
                data[0] = MAGIC_NUMBER;
                memset(data + 1, 0, DAZIBAO_HEADER_SIZE - 1);
                real_size = real_size - DAZIBAO_HEADER_SIZE;
                memcpy(data + DAZIBAO_HEADER_SIZE,
                        tlv_get_value_ptr(&tlv), real_size);
        } else {
                memcpy(data , tlv_get_value_ptr(&tlv), real_size);
        }

        tlv_destroy(&tlv);
        close(fd);
        return 0;
}
