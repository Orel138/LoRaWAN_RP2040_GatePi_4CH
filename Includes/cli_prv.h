#ifndef _CLI_PRIV
#define _CLI_PRIV

#include "semphr.h"
#include "cli.h"

/**
 * Defines the interface for different console implementations.
 */
typedef struct xConsoleIO
{
    int32_t (*read)(char * const buffer, uint32_t length);
    int32_t (*read_timeout)(char * const buffer, uint32_t length, TickType_t xTimeout);
    int32_t (*readline)(char ** const bufferPtr);
    void (*write)(const void * const pvBuffer, uint32_t length);
    void (*print)(const char * const pcString);
    void (*lock)(void);
    void (*unlock)(void);
} ConsoleIO_t;

/* Command callback prototype */
typedef void (*pdCOMMAND_LINE_CALLBACK)(ConsoleIO_t * const pxConsoleIO,
                                        uint32_t ulArgc,
                                        char * ppcArgv[]);

/* Command definition structure */
typedef struct xCOMMAND_LINE_INPUT
{
    const char * const pcCommand;
    const char * const pcHelpString;
    const pdCOMMAND_LINE_CALLBACK pxCommandInterpreter;
} CLI_Command_Definition_t;

extern char pcCliScratchBuffer[CLI_OUTPUT_SCRATCH_BUF_LEN];

/* Core CLI functions */
BaseType_t FreeRTOS_CLIRegisterCommand(const CLI_Command_Definition_t * const pxCommandToRegister);
void FreeRTOS_CLIProcessCommand(ConsoleIO_t * const pxConsoleIO, char * pcCommandInput);
const char * FreeRTOS_CLIGetParameter(const char * pcCommandString,
                                      UBaseType_t uxWantedParameter,
                                      BaseType_t * pxParameterStringLength);

/* Example commands - you can add your own */
extern const CLI_Command_Definition_t xCommandDef_ps;
extern const CLI_Command_Definition_t xCommandDef_heapStat;
extern const CLI_Command_Definition_t xCommandDef_reset;
extern const CLI_Command_Definition_t xCommandDef_clear;
extern const CLI_Command_Definition_t xCommandDef_uptime;

/* Relay command */
extern const CLI_Command_Definition_t xCommandDef_relay;

/* RAK3172 commands */
extern const CLI_Command_Definition_t xCommandDef_rakVersion;
extern const CLI_Command_Definition_t xCommandDef_rakConfig;
extern const CLI_Command_Definition_t xCommandDef_rakJoin;
extern const CLI_Command_Definition_t xCommandDef_rakSend;
extern const CLI_Command_Definition_t xCommandDef_rakAT;
extern const CLI_Command_Definition_t xCommandDef_rakReset;
extern const CLI_Command_Definition_t xCommandDef_rakTest;

#endif /* _CLI_PRIV */