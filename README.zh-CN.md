# 作业 1：四核 CPU 上的性能分析

**截止时间：10 月 6 日（星期一）晚 11:59**

**总分 100 分，附加题 6 分**

## 概述

本作业帮助你理解现代多核 CPU 中两种主要的并行执行方式：

1. 单个处理器核心内部的 SIMD 执行；
2. 使用多个核心的并行执行（你还会观察到 Intel 超线程的影响）。

你还将练习如何测量并分析并行程序的性能。这是一项困难但非常重要的技能，之后的课程中会反复用到。本作业编程量不大，但分析工作很多。

## 环境配置

本作业需要在新的 myth 机器上运行代码，主机名为 `myth[51-66].stanford.edu`。如果你在 myth 机器上没有 home 目录，请通过[这里](https://stanford.service-now.com/it_services?id=sc_cat_item&sys_id=cab169801bd918d0685d4377cc4bcbe0)提交 HelpSU 工单。

这些机器配备四核、4.2 GHz 的 Intel Core i7 处理器（动态频率调整可能在合适时提升到 4.5 GHz）。每个核心支持两个硬件线程（Intel 称之为“超线程”），并支持 AVX2 向量指令，可同时执行包含 8 个单精度值的八路 SIMD 操作。处理器完整规格见 <https://www.intel.com/content/www/us/en/products/sku/97129/intel-core-i77700k-processor-8m-cache-up-to-4-50-ghz/specifications.html>；想深入了解的同学可以阅读 [Kaby Lake 介绍](https://en.wikichip.org/wiki/intel/microarchitectures/kaby_lake)。

评分时要求报告在 Stanford myth 机器上的结果；也可以在自己的机器上运行，但必须在报告中明确注明机器类型。自己的机器需要先安装 [ISPC](http://ispc.github.io/)。

开始操作：

1. 本作业的许多程序需要 ISPC。可以按以下步骤在 myth 上安装：从 [ISPC 下载页](https://ispc.github.io/downloads.html)下载 Linux 二进制文件。2025 年秋季第 1 周可用的命令是：

    `wget https://github.com/ispc/ispc/releases/download/v1.28.1/ispc-v1.28.1-linux.tar.gz`

   解压：`tar -xvf ispc-v1.28.1-linux.tar.gz`

   将 ISPC 的 `bin` 目录加入 PATH。例如解压后目录为 `~/Downloads/ispc-v1.28.1-linux`，在 bash 中执行：

    `export PATH=$PATH:${HOME}/Downloads/ispc-v1.28.1-linux/bin`

   可以把这行加入 `.bashrc` 以永久生效。使用 csh 的同学请用 `setenv` 设置 `PATH`。

2. 从 <https://github.com/stanford-cs149/asst1> 获取起始代码：

    `git clone https://github.com/stanford-cs149/asst1.git`

## 程序 1：使用线程并行生成分形（20 分）

构建并运行 `prog1_mandelbrot_threads/` 中的代码（执行 `make` 构建，再执行 `./mandelbrot`）。程序会生成 `mandelbrot-serial.ppm`，它是著名复数集合 Mandelbrot 集的可视化结果。每个像素对应复平面中的一个值，亮度与判断该值是否属于 Mandelbrot 集所需的计算量成正比。使用 `--view 2` 可生成第二个视图；相关函数是 `mandelbrotSerial()`（位于 `mandelbrotSerial.cpp`）。

![Mandelbrot 集](handout-images/mandelbrot_viz.jpg "两个 Mandelbrot 集视图的可视化。计算像素的代价与亮度成正比。程序 1 和 3 可用 --view 2 切换到视图 2。")

使用 [std::thread](https://en.cppreference.com/w/cpp/thread/thread) 并行化图像生成。`mandelbrotThread.cpp` 中的 `mandelbrotThread()` 已给出创建一个额外线程的起始代码。主线程使用 `std::thread(function, args...)` 创建线程，并通过 `join` 等待其完成。当前新线程不做计算就立即返回；请在 `workerThreadStart` 中补充实现。本作业不需要使用其他 `std::thread` API。

**需要完成：**

1. 使用两个处理器并行生成 Mandelbrot 图像：线程 0 计算上半部分，线程 1 计算下半部分。这种将不同空间区域交给不同处理器的方式称为“空间分解”。
2. 扩展程序以支持 2、3、4、5、6、7、8 个线程，并相应划分图像块。处理器有四个核心，但每个核心支持两个超线程，因此最多可交错执行八个线程。报告中绘制**视图 1**相对于顺序参考实现的加速比随线程数变化的曲线。加速比是否线性？解释原因；也可以绘制视图 2 帮助分析，尤其注意三线程数据点。
3. 在 `workerThreadStart()` 开头和结尾加入计时代码，测量每个线程完成任务所需的时间。说明这些测量结果如何解释前面的加速比曲线。
4. 修改线程与工作之间的映射，使两个视图都达到约 7–8 倍加速（超过 7 倍也可以）。不得在线程之间使用同步。需要设计一个适用于所有线程数的统一工作分解策略，不能针对每种配置硬编码。提示：存在一种非常简单的静态分配方式，不需要线程间通信或同步。在报告中说明并行化方法，并报告最终八线程加速比。
5. 使用 16 个线程运行改进后的程序。性能是否明显优于八线程？为什么？

## 程序 2：使用 SIMD intrinsic 向量化代码（20 分）

查看 `prog2_vecintrin/main.cpp` 中的 `clampedExpSerial`。`clampedExp()` 对输入数组所有元素计算 `values[i]` 的 `exponents[i]` 次幂，并将结果限制在 9.999999。你的任务是使用 SIMD 向量指令向量化这段代码。

为简化问题，不要求直接编写映射到真实 SSE 或 AVX2 指令的 intrinsic，而是使用 `CS149intrin.h` 中定义的 CS149“伪向量 intrinsic”。该库提供作用于向量值和向量掩码的指令；它们不会转换为真实 CPU 指令，而是在库中模拟并提供更易调试的反馈。`main.cpp` 中给出了向量化 `abs()` 的例子，包含向量加载、存储和掩码操作，但该例子并不能正确处理所有输入，请自行找出原因。建议阅读 `CS149intrin.h` 中的注释和函数定义。

提示：

- 每条向量指令都有可选掩码。掩码中的 0 表示对应 lane 被屏蔽，向量操作不会覆盖其值；不提供掩码等价于全 1 掩码。你的实现需要使用多个掩码寄存器及相关操作。
- `_cs149_cntbits` 对本题很有帮助。
- 如果循环次数不是 SIMD 向量宽度的整数倍会怎样？建议用 `./myexp -s 3` 测试；`_cs149_init_ones` 可能有用。
- 使用 `./myexp -l` 可在末尾打印向量指令日志；可以调用 `addUserLog()` 添加调试信息，也可以加入 `CS149Logger.printLog()`。

程序会检查输出是否正确。如果错误，会打印第一个错误及输入输出表。`output =` 后是你的输出，应与 `gold =` 后的结果一致。程序还会输出 CS149 伪向量单元的利用率统计。性能应以 `Total Vector Instructions` 衡量（可假设每条伪向量指令耗费一个周期）；`Vector Utilization` 表示启用的向量 lane 百分比。

**需要完成：**

1. 在 `clampedExpVector` 中实现 `clampedExpSerial` 的向量化版本，必须适用于任意输入数组大小 `N` 和向量宽度 `VECTOR_WIDTH`。
2. 执行 `./myexp -s 10000`，将向量宽度设为 2、4、8、16 并记录向量利用率（修改 `CS149intrin.h` 中的 `#define VECTOR_WIDTH`）。向量宽度变化时利用率增加、减少还是不变？为什么？
3. **附加题（1 分）：** 在 `arraySumVector` 中实现 `arraySumSerial` 的向量化版本。可以假设 `VECTOR_WIDTH` 是输入数组大小 `N` 的因子。目标运行时间为 `(N / VECTOR_WIDTH + VECTOR_WIDTH)`，甚至 `(N / VECTOR_WIDTH + log2(VECTOR_WIDTH))`；`hadd` 和 `interleave` 可能有帮助。

## 程序 3：使用 ISPC 并行生成分形（20 分）

程序 3 与程序 1 一样计算 Mandelbrot 图像，但同时利用 CPU 的四个核心以及每个核心的 SIMD 单元，因此可以获得更高加速比。

程序 1 通过为每个处理器核心创建一个线程并显式分配工作。程序 3 则使用 ISPC 语言描述“相互独立的计算”：计算每个像素的值彼此独立，可以并行执行。ISPC 编译器和运行时负责高效利用 CPU 的并行资源。

你需要修复程序 3 中 C++ 与 ISPC 代码之间的一个简单错误。该错误影响性能但不影响正确性。修复后，性能应超过原始顺序 `mandelbrotSerial()` 的 32 倍。

### 程序 3 第一部分：ISPC 基础（20 分中的 10 分）

阅读 ISPC 代码时要注意：虽然语法类似 C/C++，但执行模型不同。ISPC 程序的多个 program instance 总是在 CPU 的 SIMD 执行单元上并行运行；同时执行的实例数由编译器根据目标机器决定，可通过内置变量 `programCount` 获取。当前实例编号由 `programIndex` 获取。从 C 调用 ISPC 函数可以理解为生成一组并发运行的 ISPC 实例（称为 gang）；gang 完成后才返回 C 代码。

**请停一下，重新阅读上一段。这很重要。**

ISPC 示例中，C 代码调用 `sum()`，ISPC 代码对两个 1024 元素向量求和：

```cpp
export sum(uniform int N, uniform float* a, uniform float* b, uniform float* c)
{
  // 假设 programCount 可以整除 N。
  for (int i=0; i<N; i+=programCount)
  {
    c[programIndex + i] = a[programIndex + i] + b[programIndex + i];
  }
}
```

该程序在 program instances 之间交错处理数组元素，类似程序 1 中把图像静态分配给线程。更方便且更强大的方式是只描述独立工作：ISPC 的 `foreach` 构造表达一个所有迭代彼此独立的迭代空间，ISPC 自动把迭代分配给并发实例。例如：

```cpp
export sum2(uniform int N, uniform float* a, uniform float* b, uniform float* c)
{
  foreach (i = 0 ... N)
  {
    c[i] = a[i] + b[i];
  }
}
```

建议先阅读 [ISPC walkthrough](http://ispc.github.io/example.html)。其中的示例几乎与程序 3 的 `mandelbrot_ispc()` 相同。

**需要完成：**

1. 编译运行 `mandelbrot` ISPC 程序。当前 ISPC 编译器生成八路 AVX2 指令。根据 CPU 特性，预期最大加速比是多少？为什么实际值可能更低？请考虑计算特征以及图像中对 SIMD 不友好的区域，并比较不同视图的性能。这里的 gang 映射到单个核心上的 SIMD 指令，与程序 1 使用多个核心的方式不同。

### 程序 3 第二部分：ISPC tasks（20 分中的 10 分）

ISPC 的 SPMD 模型和 `foreach` 支持 SIMD；ISPC tasks 则提供在多个核心上执行 ISPC 计算的机制。`mandelbrot_ispc_withtasks` 中的 `launch[2]` 会启动两个 task，每个 task 由一组 ISPC program instances 执行并负责图像的一部分。task 可以以任意顺序执行，也可以在不同 CPU 核心上并行执行。

**需要完成：**

1. 使用 `--tasks` 运行 `mandelbrot_ispc`。视图 1 的加速比是多少？相对于不使用 tasks 的版本提升多少？
2. 只修改 `mandelbrot_ispc_withtasks()`，调整 task 数量，使性能超过顺序版本 32 倍。你如何确定 task 数量？为什么该数量最好？
3. **附加题（2 分）：** 比较程序 1 的线程抽象与 ISPC task 抽象。创建/连接和启动/同步在语义上有什么区别？例如启动 10,000 个 ISPC tasks 与启动 10,000 个线程会怎样？请从一般情况讨论，不要只针对 Mandelbrot。

一个值得思考的问题：为什么 ISPC 同时提供 `foreach` 和 `launch` 两种表达独立并行工作的机制？为什么不让系统把 `foreach` 的迭代分到所有核心，同时为各核心生成 SIMD 代码？答案有很多，欢迎参加 office hour 讨论。

## 程序 4：迭代计算 `sqrt`（15 分）

程序 4 使用 ISPC 计算 2,000 万个 0 到 3 之间的随机数的平方根。它使用 Newton 方法迭代求解 `1/x^2 - S = 0`，初始猜测为 1.0。图中展示了 0–3 范围内达到精度所需的迭代次数；初始猜测越接近结果，收敛越快。输入为 1 时几乎立即收敛，输入接近 0 或 3 时需要更多迭代。

![sqrt 收敛图](handout-images/sqrt_graph.jpg "初始猜测为 1.0 时，sqrt 在 0–3 范围内的收敛情况。")

**需要完成：**

1. 构建运行 `sqrt`。报告单个 CPU 核心（无 tasks）和所有核心（使用 tasks）时 ISPC 的加速比。SIMD 并行和多核并行分别带来多少加速？
2. 修改 `values` 数组，使 ISPC 相对顺序实现的加速比最大，并报告有/无 tasks 的结果。是否提高了 SIMD 加速？是否提高了多核加速（从无 tasks 到有 tasks）？解释原因。
3. 构造使无 tasks ISPC 相对顺序实现加速比最小的输入，说明选择原因并报告性能。效率损失的原因是什么？注意使用 `--target=avx2`，会生成八路 SIMD 指令。
4. **附加题（最多 2 分）：** 使用 AVX2 intrinsic 手写 `sqrt`，使其接近或快于 ISPC 生成的二进制。可参考 [Intel Intrinsics Guide](https://software.intel.com/sites/landingpage/IntrinsicsGuide/)。

## 程序 5：BLAS `saxpy`（10 分）

程序 5 实现 BLAS（Basic Linear Algebra Subprograms）中的 `saxpy`。它计算 `result = scale*X+Y`，其中 `X`、`Y` 和 `result` 都是长度为 `N` 的向量（本程序中 `N = 2,000 万`）。每三个元素执行一次乘法和一次加法。`saxpy` 易于并行化，数据访问规则且执行代价可预测。

**需要完成：**

1. 编译运行 `saxpy`，比较 ISPC 无 tasks 和有 tasks 的性能。使用 tasks 带来多少加速？解释原因。是否可以显著改进，例如实现接近线性加速？请说明理由。
2. **附加题（1 分）：** `main.cpp` 中总内存带宽为 `TOTAL_BYTES = 4 * N * sizeof(float)`。虽然每个元素只从 X、Y 各加载一次并向 result 写一次，为什么乘数 4 是正确的？提示：考虑 CPU cache。
3. **附加题：** 改进 `saxpy` 性能。需要显著提升，而不是几个百分点；说明方法以及这些机器上理论上最好的实现可能达到什么水平。

## 程序 6：加速 K-Means（15 分）

程序 6 使用 K-Means 聚类算法对 100 万个数据点进行聚类（参考 [Wikipedia](https://en.wikipedia.org/wiki/K-means_clustering) 和 [CS 221 讲义](https://stanford.edu/~cpiech/cs221/handouts/kmeans.html)）。算法从 K 个中心点出发，反复更新中心直到收敛。起始和结束状态可由图像查看，红星为中心，数据点颜色表示所属簇。

起始代码正确但还不够快。你的任务是找出性能热点并改进。首先插入计时代码，使用 `common/CycleTimer.h` 中返回秒数的 `CycleTimer::currentSeconds()` 测量各部分耗时；然后基于测量结果选择优化方向。

**需要完成：**

1. 在 `prog6_kmeans` 中执行 `ln -s /afs/ir.stanford.edu/class/cs149/data/data.dat ./data.dat` 创建数据集符号链接。数据约 800 MB，这是推荐方式；也可从 myth 复制。编译运行 `kmeans`，记录总运行时间。
2. 执行 `pip install -r requirements.txt`，然后运行 `python3 plot.py`，根据 `start.log` 和 `end.log` 生成 `start.png`、`end.png`。降维可视化后并非所有点都看起来离最近中心最近，这是正常的；只要聚类总体合理即可。
3. 用计时函数定位性能瓶颈。
4. 改进实现，目标加速比约为 2.1 倍或更高（`oldRuntime / newRuntime >= 2.1`）。报告测量、推断、尝试和结果的过程。

约束：只能修改 `kmeansThread.cpp`；不能修改 `stoppingConditionMet`，也不能改变 `kMeansThread` 接口。不得改变算法功能。只能并行化以下函数之一：`dist`、`computeAssignments`、`computeCentroids`、`computeCost`。可以参考程序 1 中使用 `std::thread` 的方式。

提示：此题不需要大量代码；定位热点后，理解 K、M、N 的相对大小，并选择收益最高的并行维度。即使没有达到性能目标，只要报告体现出认真、清晰的调试过程，也能获得大部分分数。

## 关于 ARM Mac

可以使用 Apple ARM 笔记本的同学参考 [README_aarch64.md](README_aarch64.md)，报告各程序在 Apple ARM 机器上的性能和 SIMD 加速。没有现代 MacBook 的同学也可以尝试云服务商提供的 ARM 服务器，但课程组未测试。请不要将 ARM 机器上运行的代码提交到 Gradescope。

## 感兴趣的同学（强烈推荐）

ISPC 的两位作者之一 Matt Pharr 写过一篇关于其发展历史的精彩文章：[The story of ispc](https://pharr.org/matt/blog/2018/04/30/ispc-all)。文章讨论了并行系统设计、编程语言范围，以及“为什么编译器不能自动并行化程序”等问题，非常值得 CS149 学生阅读。

## 提交说明

通过 [Gradescope](https://www.gradescope.com) 提交，每组只需提交一次，但必须在提交中填写搭档姓名。需要提交两个部分：`Assignment 1 (Write-Up)` 和 `Assignment 1 (Code)`。

写报告部分：上传 `writeup.pdf`，并确保包含两位成员的姓名和 SUNet ID（两人组）。

代码部分：上传程序 2 的 `main.cpp`（文件名为 `prob2.cpp`）、程序 6 的 `kmeansThread.cpp`（文件名为 `prob6.cpp`），以及附加题所需的其他代码。请在报告中注明附加题内容。所有代码必须能在 myth 机器上直接编译运行。

## 资源与注意事项

- ISPC 文档和示例：<http://ispc.github.io/>
- Mandelbrot 图像的不同区域很值得探索。
- Intel 的 AVX2 资料：<http://software.intel.com/en-us/avx/>
- [Intel Intrinsics Guide](https://software.intel.com/sites/landingpage/IntrinsicsGuide/) 非常有用。
