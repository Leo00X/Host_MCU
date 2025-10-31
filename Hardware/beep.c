#include "ht32.h"
#include "beep.h"
#include "delay.h"


void beep_init(void)
{
	
	//使能PC端口的时钟
	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	CKCUClock.Bit.PA         = 1;
	CKCUClock.Bit.AFIO       = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);

	//配置端口功能为GPIO
	AFIO_GPxConfig(GPIO_PA, GPIO_PIN_10, AFIO_FUN_GPIO);

	//配置IO口为输出模式                                                     
	GPIO_DirectionConfig(HT_GPIOA, GPIO_PIN_10, GPIO_DIR_OUT);

	//默认输出0
	GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_10, SET);

}

void beep_on(void)
{
	GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_10, RESET );	

}
void beep_off(void)
{
	GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_10, SET );	
}

void beep_key(void)
{
	GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_10, RESET );	
	delay_ms(30);
	GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_10, SET );	
}

// 外部传感器变量声明 (由其他模块更新)
// n1: 湿度 (Humidity)
// n2: 烟雾 (Smoke)
// n3: 可燃气体 (Combustible Gas)
// n4: 温度 (Temperature)
extern int n1, n2, n3, n4;

// 定义报警模式的结束标记
#define END_OF_PATTERN 0

// ---------------- Threshold Definitions (示例值, 请根据实际传感器调整) ----------------
#define TEMPERATURE_THRESHOLD_HIGH 33  // 温度超标阈值 (例如: 35°C) - 对应 n4
#define HUMIDITY_THRESHOLD_HIGH 94     // 湿度超标阈值 (例如: 80% RH) - 对应 n1
#define SMOKE_DETECT_THRESHOLD 4     // 烟雾检测阈值 (例如: 传感器ADC读数或其他单位) - 对应 n2
#define GAS_DETECT_THRESHOLD 90       // 可燃气体检测阈值 (例如: 传感器ADC读数或其他单位) - 对应 n3

// ---------------- Alarm Pattern Definitions (单位: 毫秒) ----------------
// 格式: {响时间1, 停时间1, 响时间2, 停时间2, ..., END_OF_PATTERN}

// 一级告警（湿度超标）：响0.5秒 → 停0.5秒
const unsigned int PATTERN_HUMIDITY[] = {500, 500, END_OF_PATTERN};

// 二级告警（温度超标）：响1秒 → 停0.3秒
const unsigned int PATTERN_TEMPERATURE[] = {1000, 300, END_OF_PATTERN};

// 三级告警（烟雾检测）：自定义模式，例如短促多次后长响 (更紧急)
const unsigned int PATTERN_SMOKE[] = {750, 150, 750, 150, 1500, 150, END_OF_PATTERN};

// 四级告警（可燃气体检测）：自定义模式，例如非常急促的短响 (最紧急)
const unsigned int PATTERN_GAS[] = {200, 100, 200, 100, 200, 100, 1000, 100, END_OF_PATTERN};

// ---------------- Number of Repetitions for each alarm pattern cycle ----------------
// 定义每种警报模式在一个警报周期内重复鸣响的次数
#define REPETITIONS_HUMIDITY    2 // 湿度报警响3次 (0.5s on, 0.5s off) * 3
#define REPETITIONS_TEMPERATURE 2 // 温度报警响3次 (1s on, 0.3s off) * 3
#define REPETITIONS_SMOKE       2 // 烟雾报警重复2次定义的复杂模式
#define REPETITIONS_GAS         2 // 燃气报警重复2次定义的复杂模式

// 枚举定义报警类型
typedef enum {
    ALARM_MODE_NONE,         // 无报警
    ALARM_MODE_HUMIDITY,     // 湿度报警 (n1)
    ALARM_MODE_TEMPERATURE,  // 温度报警 (n4)
    ALARM_MODE_SMOKE,        // 烟雾报警 (n2)
    ALARM_MODE_GAS           // 可燃气体报警 (n3)
} AlarmMode;

/**
 * @brief 播放指定的报警模式 (阻塞式)
 * @param pattern_ms 报警模式数组 {响时间, 停时间, ... END_OF_PATTERN}
 * @param repetitions 整个模式的重复次数
 */
void play_alarm_pattern_blocking(const unsigned int* pattern_ms, int repetitions) {
    if (repetitions <= 0 || pattern_ms == 0) { // C中检查指针是否为NULL应为 pattern_ms == NULL
        return;
    }

    for (int rep = 0; rep < repetitions; ++rep) {
        int k = 0;
        while (pattern_ms[k] != END_OF_PATTERN) {
            // 蜂鸣器响阶段
            if (pattern_ms[k] > 0) {
                beep_on();
                delay_ms(pattern_ms[k]);
            }
            k++;

            // 检查是否模式在“响”阶段后就结束了
            if (pattern_ms[k] == END_OF_PATTERN) {
                beep_off(); // 确保在模式结束后关闭蜂鸣器
                break;
            }

            // 蜂鸣器停阶段
            if (pattern_ms[k] > 0) {
                beep_off();
                delay_ms(pattern_ms[k]);
            }
            k++;
        }
    }
    beep_off(); // 确保在所有重复结束后关闭蜂鸣器
}

/**
 * @brief 处理传感器数据并触发相应的动态报警
 * 此函数应在主循环中定期调用。
 * 它会根据传感器读数决定最高优先级的报警并播放一次报警序列。
 */
void process_dynamic_alarms() {
    AlarmMode active_alarm_to_play = ALARM_MODE_NONE;


    // 根据确定的报警类型播放相应的报警模式
    switch (active_alarm_to_play) {
        case ALARM_MODE_GAS: // n3
            play_alarm_pattern_blocking(PATTERN_GAS, REPETITIONS_GAS);
            // 可以在特定警报后添加额外的延时，如果需要的话
            // delay_ms(2000); // 例如，燃气报警后强制静默2秒
            break;
        case ALARM_MODE_SMOKE: // n2
            play_alarm_pattern_blocking(PATTERN_SMOKE, REPETITIONS_SMOKE);
            // delay_ms(1500);
            break;
        case ALARM_MODE_TEMPERATURE: // n4
            play_alarm_pattern_blocking(PATTERN_TEMPERATURE, REPETITIONS_TEMPERATURE);
            // delay_ms(1000); // 例如，温度报警后强制静默1秒
            break;
        case ALARM_MODE_HUMIDITY: // n1
            play_alarm_pattern_blocking(PATTERN_HUMIDITY, REPETITIONS_HUMIDITY);
            // delay_ms(1000);
            break;
        case ALARM_MODE_NONE:
            // 没有活动的警报，确保蜂鸣器是关闭的
            beep_off();
            // 此处可以根据需要执行“一切正常”的逻辑
            break;
    }

    // 如果在任何警报播放后都需要一个统一的静默期，可以在这里添加。
    // 但请注意，这会使系统在静默期内不响应新的报警检测。
    // 更优的做法可能是在主循环中控制 `process_dynamic_alarms` 的调用频率。
    // if (active_alarm_to_play != ALARM_MODE_NONE) {
    //     delay_ms(500); // 例如，任何报警序列播放后，至少静默0.5秒
    // }
}
