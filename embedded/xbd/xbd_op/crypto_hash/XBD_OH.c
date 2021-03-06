#include <stdint.h>
#include <strings.h>
#include "crypto_hash.h"
#include "XBD_commands.h"
#include "XBD_debug.h"
#include "XBD_operation.h"
#include "XBD_OH.h"
#include "try.h"

#ifdef I_AM_OS_BASED
static volatile uint32_t burner;
static volatile uint32_t afterburner;
#endif	

/** 
 * Executes an operation 
 *
 * @param parameterType The type of parameters currently in the parameter buffer
 * @param parameterBuffer [4 bytes length, network order][data bytes]
 * @param parameterBufferLen Length of parameterBuffer
 * @param resultBuffer return [4 bytes network order return code][crypto_hash_BYTES hash value]
 * @param p_stackUse pointer to a global variable to report the amount of stack used
 * @param result_len  Length of data in resultBuffer
 * @returns 0 if success, else errorcode from operation
 */
int32_t OH_handleExecuteRequest(uint32_t parameterType, uint8_t *parameterBuffer, size_t parameterBufferLen, uint8_t* resultBuffer, uint32_t *p_stackUse, size_t *result_len) {

    if( CRYPTO_HASH_TYPE != parameterType ) {
        XBD_DEBUG("Rec'd W-R-O-N-G EXecute req:");
        XBD_DEBUG("\nparameterType="); XBD_DEBUG_32B(parameterType);
        *(int32_t*)resultBuffer=HTONL(FAIL_TYPE_MISMATCH);
        //prepare 'FAIL' response to XBH
        //Return only size of failure code in result buffer
        return NUMBSIZE;
    }


#ifdef XBX_DEBUG_EBASH
    XBD_DEBUG("\ninlen=");XBD_DEBUG_32B(inlen);
    XBD_DEBUG_BUF("*p_in", p_in, inlen);
#endif
    /* watch out for the dogs */
    XBD_startWatchDog(100);

    /* Prepare Stack usage measurement */
    XBD_paintStack();
    *p_stackUse=XBD_countStack();

    /*	If running on an embedded OS, sleep some time to try and get the CPU
        uninterrupted for the measurement	*/
#ifdef I_AM_OS_BASED
    for(afterburner=65;afterburner<5000000;afterburner++)
    {
        for(burner=13;burner<TC_VALUE_SEC;burner++)
        {
            burner=burner*1;
        }
    }
    usleep(TC_VALUE_SEC*100000);		
#endif	

    /** Sends the signal "start-of-execution" to the XBH */
    XBD_sendExecutionStartSignal();

    //We can use paramterBuffer directly since it only contains data to be
    //hashed with no length prefix
    int32_t ret = crypto_hash(
            resultBuffer+4,
            parameterBuffer,
            (unsigned long long) parameterBufferLen 
            );
    /** Sends the signal "end-of-execution" to the XBH */
    XBD_sendExecutionCompleteSignal();

    /* Report Stack usage measurement */
    *p_stackUse=*p_stackUse-XBD_countStack();

    *(int32_t*)resultBuffer=HTONL(ret);

    /* stop the watchdog */
    XBD_stopWatchDog();

#ifdef XBX_DEBUG_EBASH
    XBD_DEBUG("\ncrypto_hash ret="); XBD_DEBUG_BYTE(ret);
    XBD_DEBUG_BUF("resultBuffer", resultBuffer, crypto_hash_BYTES+1);
#endif

    //Set result size
    *result_len = NUMBSIZE+crypto_hash_BYTES;

    //Return return code
    return ret;
}

