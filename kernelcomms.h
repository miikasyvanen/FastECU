#ifndef KERNELCOMMS_H
#define KERNELCOMMS_H

/***********************************
 * Hitachi (M32R) commands
 * ********************************/
#define SID_HITACHI_BLOCK_READ                  0xA0
#define SID_HITACHI_ADDR_READ                   0xA8
#define SID_HITACHI_BLOCK_WRITE                 0xB0
#define SID_HITACHI_ADDR_WRITE                  0xB8
#define SID_HITACHI_FLASH_READ                  0x00//???

#define SID_HITACHI_FLASH_ERASE                 0x31//???
#define SID_HITACHI_FLASH_WRITE                 0x61
#define SID_HITACHI_FLASH_WRITE_END             0x69//???

/***********************************
 * OpenECU (HC16) kernel commands
 * ********************************/
#define SID_OE_UPLOAD_KERNEL                    0x53

#define SID_OE_KERNEL_START_COMM                0xBEEF
#define SID_OE_KERNEL_ID                        0x01
#define SID_OE_KERNEL_CRC                       0x02
#define SID_OE_KERNEL_READ_AREA                 0x03
#define SID_OE_KERNEL_PROG_VOLT                 0x04

#define SID_OE_KERNEL_FLASH_ENABLE				0x20;
#define SID_OE_KERNEL_FLASH_DISABLE				0x21;
#define SID_OE_KERNEL_WRITE_FLASH_BUFFER		0x22;
#define SID_OE_KERNEL_VALIDATE_FLASH_BUFFER		0x23;
#define SID_OE_KERNEL_COMMIT_FLASH_BUFFER       0x24;
#define SID_OE_KERNEL_BLANK_16K_PAGE            0x25;

/****************************************
 * NisProg based kernels k-line commands
 * *************************************/
#define SID_RECUID                  0x1A	/* readECUID , in this case kernel ID */
#define SID_RECUID_PRC              "\x5A"	/* positive response code, to be concatenated to version string */

#define SID_RMBA                    0x23	/* ReadMemByAddress. format : <SID_RMBA> <AH> <AM> <AL> <SIZ>  , siz <= 251. */
                                            /* response : <SID + 0x40> <D0>....<Dn> <AH> <AM> <AL> */

#define SID_WMBA                    0x3D	/* WriteMemByAddress (RAM only !) . format : <SID_WMBA> <AH> <AM> <AL> <SIZ> <DATA> , siz <= 250. */
                                            /* response : <SID + 0x40> <AH> <AM> <AL> */

#define SID_TP                      0x3E	/* TesterPresent; not required but available. */

#define SID_DUMP                    0xBD	/* format : 0xBD <AS> <BH BL> <AH AL>  ; AS=0 for EEPROM, =1 for ROM */
#define SID_DUMP_EEPROM             0x00
#define SID_DUMP_ROM                0x01
#define SID_DUMP_SUB_EEPSCI3_PJ2    0x02
#define SID_DUMP_SUB_EEPSCI3_PJ3    0x03
#define SID_DUMP_SUB_EEPSCI4_PF10   0x04

/* SID_FLASH and subcommands */
#define SID_FLASH                   0xBC	/* low-level reflash commands; only available after successful RequestDownload */
    #define SIDFL_UNPROTECT         0x55	//enable erase / write. format : <SID_FLASH> <SIDFL_UNPROTECT> <~SIDFL_UNPROTECT>
    #define SIDFL_EB                0x01	//erase block. format : <SID_FLASH> <SIDFL_EB> <BLOCK #>
    #define SIDFL_WB                0x02	//write n-byte block. format : <SID_FLASH> <SIDFL_WB> <A2> <A1> <A0> <D0>...<D(SIDFL_WB_DLEN -1)> <CRC>
                                            // Address is <A2 A1 A0>;   CRC is calculated on address + data.
    #define SIDFL_WB_DLEN           128     //bytes sent per niprog block

/* SID_CONF and subcommands */
#define SID_CONF                    0xBE    /* set & configure kernel */
    #define SID_CONF_SETSPEED       0x01	/* set comm speed (BRR divisor reg) : <SID_CONF> <SID_CONF_SETSPEED> <new divisor> */
                                            //this requires a new StartComm request at the new speed
    #define SID_CONF_SETEEPR        0x02	/* set eeprom_read() function address <SID_CONF> <SID_CONF_SETEEPR> <AH> <AM> <AL> */
    #define SID_CONF_CKS1           0x03	//verify if 4*<CRCH:CRCL> hash is valid for 4*256B chunks of the ROM (starting at <CNH:CNL> * 1024)
                                            //<SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
    #define SID_CONF_R16            0x04    /* for debugging : do a 16bit access read at given adress in RAM (top byte 0xFF)
                                             * <SID_CONF> <SID_CONF_R16> <A2> <A1> <A0> */

#define ROMCRC_NUMCHUNKS_16BIT      1
#define ROMCRC_CHUNKSIZE_16BIT      16384
#define ROMCRC_ITERSIZE_16BIT       (ROMCRC_NUMCHUNKS_16BIT * ROMCRC_CHUNKSIZE_16BIT)
#define ROMCRC_LENMASK_16BIT        ((ROMCRC_NUMCHUNKS_16BIT * ROMCRC_CHUNKSIZE_16BIT) - 1)  //should look like 0x3FF

#define ROMCRC_NUMCHUNKS_32BIT      4
#define ROMCRC_CHUNKSIZE_32BIT      256
#define ROMCRC_ITERSIZE_32BIT       (ROMCRC_NUMCHUNKS_32BIT * ROMCRC_CHUNKSIZE_32BIT)
#define ROMCRC_LENMASK_32BIT        ((ROMCRC_NUMCHUNKS_32BIT * ROMCRC_CHUNKSIZE_32BIT) - 1)  //should look like 0x3FF

#define ROMCRC_NUMCHUNKS_CAN        1
#define ROMCRC_CHUNKSIZE_CAN        256
#define ROMCRC_ITERSIZE_CAN         (ROMCRC_NUMCHUNKS_CAN * ROMCRC_CHUNKSIZE_CAN)
#define ROMCRC_LENMASK_CAN          ((ROMCRC_NUMCHUNKS_CAN * ROMCRC_CHUNKSIZE_CAN) - 1)  //should look like 0x3FF


#define SID_FLREQ                   0x34	/* RequestDownload */
#define SID_STARTCOMM               0x81    /* startCommunication */
#define SID_KERNEL_INIT             0x81    /* kernel init, same as startcomm */

#define SID_RESET                   0x11	/* restart ECU */

/*************************************
 * NisProg based kernels CAN commands
 * **********************************/
#define SID_CAN_START_COMM          0x7A
#define SID_CAN_ENTER_BL            0xFF86
#define SID_CAN_CHECK_COMM_BL       0x90

#define SID_CAN_DUMP_ROM            0xD8
#define SID_CAN_DUMP_EEPROM         0xB8

#define SID_KERNEL_ADDRESS          0x98
#define SID_KERNEL_CHECKSUM         0xB0
#define SID_KERNEL_JUMP             0xA0

#define SID_KERNEL_STOP_CAN         0xFFC8

/* SID_FLASH and subcommands */
#define SID_CAN_FLASH               0xE0	/* low-level reflash commands; only available after successful RequestDownload */
    #define SID_CAN_FL_UNPROTECT    0xA5	//enable erase / write.
    #define SID_CAN_FL_PROTECT      0x00	//disable erase / write.
    #define SID_CAN_FL_EB           0xF0	//erase block. format : <SID_FLASH> <SIDFL_EB> <BLOCK #>
    #define SID_CAN_FL_WB           0xF8

/* SID_CONF and subcommands */
#define SID_CAN_CONF_CKS            0xD0


/* SID_CONF error codes */
#define SID_CONF_CKS_BADCKS     	0x77	//NRC when crc is bad


/**** Common flash error codes for all platforms. */

/* some standard iso14230 errors */
#define ISO_NRC_GR	0x10	/* generalReject */
#define ISO_NRC_SNS	0x11	/* serviceNotSupported */
#define ISO_NRC_SFNS_IF	0x12	/* subFunctionNotSupported-Invalid Format */
#define ISO_NRC_CNCORSE	0x22	/* conditionsNoteCorrectOrRequestSequenceError */
#define ISO_NRC_IK	0x35	/* invalidKey */
#define ISO_NRC_CNDTSA	0x42	/* canNotDownloadToSpecifiedAddress */


/* Custom errors adjusted to fit with 180nm error codes (different from possible FPFR return values)
 * and serve as the iso14230 NRC
 *
 * i.e. FPFR has bits 0..2 defined. Map those errors as (0x80 | FPFR)
 */
#define PF_ERROR 0x80		//generic flashing error : FWE, etc

// FPFR block : 0x81-0x87 (180nm only)
// keep only bits 1,2 ? (&= 0x06)
#define PF_FPFR_BASE 0x80		//just for |= op
#define PF_FPFR_BADINIT 0x81	//initialization failed
#define PF_FPFR_FQ 0x82		//Setting of operating freq abnormal
#define PF_FPFR_UB 0x84		//user branch setting abnormal (not used)
//0x85 combined errors, not sure if possible
//0x86 ..
//0x87 ..

/****  general block */
#define PFWB_OOB 0x88		//dest out of bounds
#define PFWB_MISALIGNED 0x89	//dest not on 128B boundary
#define PFWB_LEN 0x8A		//len not multiple of 128
#define PFWB_VERIFAIL 0x8B	//post-write verify failed

#define PFEB_BADBLOCK 0x8C	//erase: bad block #
#define PFEB_VERIFAIL 0x8D	//erase verify failed

#define PF_SILICON 0x8E	//Not running on correct silicon (180 / 350nm)

/**** 7055 (350nm) codes  */
#define PFWB_MAXRET 0x8F	//max # of rewrite attempts

#define SID_CONF_LASTERR 0x05	// get last kernel internal error code then clear to 0 (ERR_OK)

/**** consult-2 codes : defines at least 0x90-0x95. skip those */
//0x90
//..
//0x9F

/**** 7051 (350nm) codes */
#define PF_ERROR_AFTERASE 0xA0
#define PF_ERROR_B4WRITE 0xA1
#define PF_ERROR_AFTWRITE 0xA2
#define PF_ERROR_VERIF 0xA3
//0xA4
//..
//0xA7

/**** 180nm SID_FLREQ ( RequestDownload) codes */
#define SID34_BADFCCS	0xA8
#define SID34_BADRAMER	0xA9
#define SID34_BADDL_ERASE	0xAA
#define SID34_BADDL_WRITE	0xAB
#define SID34_BADINIT_ERASE	0xAC
#define SID34_BADINIT_WRITE	0xAD
#define SID34_BADINIT_TDER 0xAE
#define SID34_BADINIT_FTDAR 0xAF

/**** 180nm SID_FLREQ DPFR codes
 * actual DPFR value is masked to keep bits 1,2 then |= 0xB0 .
 */
#define SID34_DPFR_BASE 0xB0	//just for |= op
#define SID34_DPFR_SF 0xB1	//success/fail... never set by itself ?
#define SID34_DPFR_FK 0xB2	//flash key register error
#define SID34_DPFR_SS 0xB4	//source select error
//0xB5 combined errrors
//0xB6 ..
//0xB7 ..

#endif // KERNELCOMMS_H
