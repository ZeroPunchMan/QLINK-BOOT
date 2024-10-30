#include "helper.h"
#include "cl_log.h"

//***********************状态滤波****************************
bool StatusFilter(StatusFilter_t *filter, bool newStatus)
{
    CL_QueueInfo_t *q = filter->queue;

    if (CL_QueueFull(q))
    {
        bool oldVal;
        CL_QueuePoll(q, &oldVal);

        if (oldVal)
            filter->count--;
    }

    CL_QueueAdd(q, &newStatus);
    if (newStatus)
        filter->count++;

    if (filter->status)
    {
        if (filter->count <= CL_QueueCapacity(q) / 2)
            filter->status = false;
    }
    else
    {
        if (filter->count >= CL_QueueCapacity(q))
            filter->status = true;
    }

    return filter->status;
}


//------------------------------------------------------------------------

//**************************NTC************************************

#define NTC_R (1000)
#define NTC_TO_ADC(n) (n * 4096UL / (n + NTC_R))

#define NTC_BASE_CELSIUS (-10)

const uint32_t ntcAdcTable[] = {
    NTC_TO_ADC(55249.1),
    NTC_TO_ADC(52330.7),
    NTC_TO_ADC(49584.1),
    NTC_TO_ADC(46998.2),
    NTC_TO_ADC(44562.7),
    NTC_TO_ADC(42268),
    NTC_TO_ADC(40105.2),
    NTC_TO_ADC(38065.8),
    NTC_TO_ADC(36142.3),
    NTC_TO_ADC(34327.4),
    NTC_TO_ADC(32614.2),
    NTC_TO_ADC(30996.6),
    NTC_TO_ADC(29468.8),
    NTC_TO_ADC(28025.1),
    NTC_TO_ADC(26660.6),
    NTC_TO_ADC(25370.4),
    NTC_TO_ADC(24150.1),
    NTC_TO_ADC(22995.5),
    NTC_TO_ADC(21902.8),
    NTC_TO_ADC(20868.2),
    NTC_TO_ADC(19888.4),
    NTC_TO_ADC(18960.2),
    NTC_TO_ADC(18080.6),
    NTC_TO_ADC(17246.7),
    NTC_TO_ADC(16456.1),
    NTC_TO_ADC(15706.1),
    NTC_TO_ADC(14994.5),
    NTC_TO_ADC(14319.1),
    NTC_TO_ADC(13677.9),
    NTC_TO_ADC(13069),
    NTC_TO_ADC(12490.5),
    NTC_TO_ADC(11940.9),
    NTC_TO_ADC(11418.4),
    NTC_TO_ADC(10921.7),
    NTC_TO_ADC(10449.4),
    NTC_TO_ADC(10000),
    NTC_TO_ADC(9572.4),
    NTC_TO_ADC(9165.4),
    NTC_TO_ADC(8777.9),
    NTC_TO_ADC(8408.9),
    NTC_TO_ADC(8057.4),
    NTC_TO_ADC(7722.5),
    NTC_TO_ADC(7403.3),
    NTC_TO_ADC(7099),
    NTC_TO_ADC(6808.8),
    NTC_TO_ADC(6532.1),
    NTC_TO_ADC(6268),
    NTC_TO_ADC(6016),
    NTC_TO_ADC(5775.5),
    NTC_TO_ADC(5545.9),
    NTC_TO_ADC(5326.6),
    NTC_TO_ADC(5117.1),
    NTC_TO_ADC(4916.9),
    NTC_TO_ADC(4725.7),
    NTC_TO_ADC(4542.8),
    NTC_TO_ADC(4368),
    NTC_TO_ADC(4200.8),
    NTC_TO_ADC(4040.9),
    NTC_TO_ADC(3887.8),
    NTC_TO_ADC(3741.4),
    NTC_TO_ADC(3601.2),
    NTC_TO_ADC(3467),
    NTC_TO_ADC(3338.5),
    NTC_TO_ADC(3215.4),
    NTC_TO_ADC(3097.5),
    NTC_TO_ADC(2984.5),
    NTC_TO_ADC(2876.2),
    NTC_TO_ADC(2772.3),
    NTC_TO_ADC(2672.7),
    NTC_TO_ADC(2577.2),
    NTC_TO_ADC(2485.6),
    NTC_TO_ADC(2397.7),
    NTC_TO_ADC(2313.4),
    NTC_TO_ADC(2232.4),
    NTC_TO_ADC(2154.7),
    NTC_TO_ADC(2080.1),
    NTC_TO_ADC(2008.4),
    NTC_TO_ADC(1939.6),
    NTC_TO_ADC(1873.4),
    NTC_TO_ADC(1809.8),
    NTC_TO_ADC(1748.7),
    NTC_TO_ADC(1690),
    NTC_TO_ADC(1633.5),
    NTC_TO_ADC(1579.2),
    NTC_TO_ADC(1526.9),
    NTC_TO_ADC(1476.7),
    NTC_TO_ADC(1428.3),
    NTC_TO_ADC(1381.8),
    NTC_TO_ADC(1336.9),
    NTC_TO_ADC(1293.8),
    NTC_TO_ADC(1252.3),
    NTC_TO_ADC(1212.3),
    NTC_TO_ADC(1173.7),
    NTC_TO_ADC(1136.6),
    NTC_TO_ADC(1100.8),
    NTC_TO_ADC(1066.3),
    NTC_TO_ADC(1033.1),
    NTC_TO_ADC(1001.1),
    NTC_TO_ADC(970.2),
    NTC_TO_ADC(940.4),
    NTC_TO_ADC(911.6),
    NTC_TO_ADC(883.9),
    NTC_TO_ADC(857.2),
    NTC_TO_ADC(831.4),
    NTC_TO_ADC(806.4),
    NTC_TO_ADC(782.4),
    NTC_TO_ADC(759.2),
    NTC_TO_ADC(736.7),
    NTC_TO_ADC(715.1),
    NTC_TO_ADC(694.2),
    NTC_TO_ADC(673.9),
};

int16_t NtcAdcToTemp_10K(uint32_t adc)
{
    int16_t offset = INT16_MAX;
    for (int i = 0; i < CL_ARRAY_LENGTH(ntcAdcTable); i++)  //todo 二分查找
    {
        if(adc > ntcAdcTable[i])
        {
            offset = i;
            break;
        }
    }

    if(offset != INT16_MAX)
        return NTC_BASE_CELSIUS + offset;
    
    return NTC_BASE_CELSIUS + ((int16_t)CL_ARRAY_LENGTH(ntcAdcTable)) - 1;
}

bool Ntc10kTableCheck(void)
{
    for (int i = 0; i < CL_ARRAY_LENGTH(ntcAdcTable) - 1; i++)
    {
        if (ntcAdcTable[i] < ntcAdcTable[i + 1])
            return false;
    }

    return true;
}

//---------------------------------------------------------------
