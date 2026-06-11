/*
 * filter.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "filter.h"

// ================= 一阶低通滤波实现 =================
void EmaFilter_Init(EmaFilter_t *f, float alpha) {
    f->alpha = alpha;// 设定信任系数
    f->last_out = 0.0f;// 清空历史输出
    f->is_init = 0;// 标记为“未完成第一次采样”
}

float EmaFilter_Process(EmaFilter_t *f, float new_val) {
    if (!f->is_init) {
        f->last_out = new_val; // 第一帧数据直接信任，防止启动时从0慢爬
        f->is_init = 1;
        return new_val;
    }
    // 经典的工业公式: Y(n) = a*X(n) + (1-a)*Y(n-1)
    f->last_out = (f->alpha * new_val) + ((1.0f - f->alpha) * f->last_out);
    return f->last_out;
}

// ================= 中值平均滤波实现 ================= 去掉最高和最低
void MedianFilter_Init(MedianFilter_t *f) {
    f->index = 0;// 数组写入位置的指针归零
    f->is_full = 0;// 标记数组还没填满
    for(int i=0; i<MEDIAN_SAMPLE_COUNT; i++) f->buf[i] = 0;// 清空缓存数组
}

float MedianFilter_Process(MedianFilter_t *f, float new_val) {
    f->buf[f->index++] = new_val;
    if (f->index >= MEDIAN_SAMPLE_COUNT) {//满了
        f->index = 0;
        f->is_full = 1;
    }

    if (!f->is_full) {
        return new_val; // 没攒够数据前直接输出，保证实时性
    }

    // 找出最大值、最小值并求和 (O(N) 复杂度，比冒泡排序快得多)
    //用了一次 for 循环（复杂度 $O(N)$），就同时干了三件事：找到了最高分、找到了最低分、算出了总和。最后拿总和减去最高和最低，除以剩下的个数 (N-2)。
    float max = f->buf[0], min = f->buf[0], sum = 0;
    for (int i = 0; i < MEDIAN_SAMPLE_COUNT; i++) {
        if (f->buf[i] > max) max = f->buf[i];
        if (f->buf[i] < min) min = f->buf[i];
        sum += f->buf[i];
    }
    return (sum - max - min) / (MEDIAN_SAMPLE_COUNT - 2);
}
