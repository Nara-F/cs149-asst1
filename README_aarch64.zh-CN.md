# 作业 1：四核 CPU 上的性能分析（ARM 版本）

**截止时间：10 月 3 日（星期五）晚 11:59**

**总分 100 分，附加题 6 分**

## 概述

本作业帮助你理解现代多核 CPU 中的 SIMD 执行和多核心并行，并练习测量、解释并行程序性能。

**注意：在 ARM 上运行是可选的，不计入成绩。**

## 环境配置

你需要在基于 ARM 的机器上运行，例如 Apple M 系列 Mac。首先安装 [ISPC](http://ispc.github.io/)。

从 [ISPC 下载页](https://ispc.github.io/downloads.html)下载适用于 macOS 的版本。可以使用 `curl` 或 `wget`；2025 年秋季第 1 周可用的命令是：

    `wget https://github.com/ispc/ispc/releases/download/v1.28.1/ispc-v1.28.1-MacOS.tar.gz`

解压：`tar -xvf ispc-v1.28.1-MacOS.tar.gz`

将 `bin` 目录加入 PATH。例如：

    `export PATH=$PATH:${HOME}/Downloads/ispc-v1.28.1-MacOS/bin`

随后获取起始代码：

    `git clone https://github.com/stanford-cs149/asst1.git`

## 作业内容

ARM 版本的程序 1–6 与主 README 中的任务相同，区别如下：

- 线程数量应从 2 扩展到机器上的性能核心数（或机器支持的线程数），并按图像块划分工作。
- 第 1 题中不要求固定使用 2、3、4、5、6、7、8 个线程；请根据 Mac 的性能核心数或机器线程数测试。
- 第 4 题应尽可能提升 Mandelbrot 的加速比，并用与机器性能核心数相同的线程数报告结果。
- 第 5 题使用机器线程数两倍的线程进行比较。
- 程序 2 可以使用 ARM 的 Neon SIMD 指令作为背景，但实际实现仍使用 `CS149intrin.h` 中的伪向量 intrinsic。

程序 3、4、5、6 的问题、附加题、约束、数据集、报告要求和提交方式均与 [主 README 的中文版本](README.zh-CN.md) 相同。请特别注意：程序 6 只能修改 `kmeansThread.cpp`，只能并行化 `dist`、`computeAssignments`、`computeCentroids`、`computeCost` 中的一个，并且不能改变算法结果。

## ARM 版本报告

请报告各程序在 Apple ARM 笔记本上的性能，并说明 SIMD 执行带来的加速。没有现代 MacBook 的同学可以尝试云服务商提供的 ARM 服务器，但课程组未测试。请不要将 ARM 机器上运行的代码提交到 Gradescope。

其余资源、ISPC 介绍、提交说明和附加题要求请参见 [README.zh-CN.md](README.zh-CN.md)。
