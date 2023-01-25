
#ifndef STM_BOOTLOADER_REGISTERS_H_
#define STM_BOOTLOADER_REGISTERS_H_

#define BL_MAX_MESSAGE_LEN      256U


#define BL_GET_COMMAND          0x00U  //! Get CMD command
#define BL_GET_VER_COMMAND      0x01U  //! Get Version command
#define BL_GET_ID_COMMAND       0x02U  //! Get ID command
#define BL_RMEM_COMMAND         0x11U  //! Read Memory command
#define BL_GO_COMMAND           0x21U  //! Go command
#define BL_WMEM_COMMAND         0x31U  //! Write Memory command
#define BL_NS_WMEM_COMMAND      0x32U  //! No-Stretch Write Memory command
#define BL_EMEM_COMMAND         0x44U  //! Erase Memory command
#define BL_NS_EMEM_COMMAND      0x45U  //! No-Stretch Erase Memory command
#define BL_WP_COMMAND           0x63U  //! Write Protect command
#define BL_NS_WP_COMMAND        0x64U  //! No-Stretch Write Protect command
#define BL_WU_COMMAND           0x73U  //! Write Unprotect command
#define BL_NS_WU_COMMAND        0x74U  //! No-Stretch Write Unprotect command
#define BL_RP_COMMAND           0x82U  //! Readout Protect command
#define BL_NS_RP_COMMAND        0x82U  //! No-Stretch Readout Protect command
#define BL_RU_COMMAND           0x92U  //! Readout Unprotect command
#define BL_NS_RU_COMMAND        0x93U  //! No-Stretch Readout Unprotect command

#define BL_ACK_CODE             0x79U //! Acknowledgment code
#define BL_NACK_CODE            0x1FU //! None Acknowledgment code
#define BL_BUSY_CODE            0x77U //! Bootloader busy code



#endif /* STM_BOOTLOADER_REGISTERS_H_ */