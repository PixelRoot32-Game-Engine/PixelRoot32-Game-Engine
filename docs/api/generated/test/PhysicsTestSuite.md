# PhysicsTestSuite

<Badge type="info" text="Class" />

**Source:** `PhysicsTestSuite.h`

## Description

Comprehensive testing for Flat Solver

## Methods

### `static TestResult testDeterminism(int numRuns = 100, int numFrames = 600)`

### `static TestResult testEnergyConservation(int numFrames = 1000)`

### `static TestResult testStackingStability(int numBoxes = 10, int numFrames = 600)`

### `static TestResult testBounceConsistency(int numBounces = 100)`

### `static TestResult testPerformance(int numBodies = 20, int numFrames = 600)`

### `static void runAllTests()`
