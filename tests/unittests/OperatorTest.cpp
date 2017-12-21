// Copyright 2017 Facebook Inc.  All Rights Reserved.

#include "glow/ExecutionEngine/ExecutionEngine.h"
#include "glow/Graph/Graph.h"
#include "glow/IR/IR.h"
#include "glow/IR/IRBuilder.h"
#include "glow/IR/Instrs.h"

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <string>

using namespace glow;

TEST(Operator, matmul) {
  ExecutionEngine EE;

  auto &G = EE.getGraph();

  auto *batch = G.createVariable(ElemKind::FloatTy, {1, 2, 3}, "batch");
  auto *filter = G.createVariable(ElemKind::FloatTy, {3, 2}, "filter");
  auto *result = G.createVariable(ElemKind::FloatTy, {1, 2, 2}, "result");
  batch->getPayload().getHandle() = {1, 2, 3, 4, 5, 6};
  filter->getPayload().getHandle() = {7, 8, 9, 10, 11, 12};

  auto R = G.createBatchedMatMul("MM", batch, filter);

  G.createSave("save", R, result);

  EE.compile(CompilationMode::Infer);

  EE.run({}, {});

  auto H = result->getPayload().getHandle();
  EXPECT_NEAR(H.at({0, 0, 0}), 58, 0.001);
  EXPECT_NEAR(H.at({0, 0, 1}), 64, 0.001);
  EXPECT_NEAR(H.at({0, 1, 0}), 139, 0.001);
  EXPECT_NEAR(H.at({0, 1, 1}), 154, 0.001);
}

TEST(Operator, batched_reduce_add) {
  ExecutionEngine EE;

  auto &G = EE.getGraph();

  auto *batch = G.createVariable(ElemKind::FloatTy, {2, 4}, "batch");
  auto *result = G.createVariable(ElemKind::FloatTy, {4}, "result");
  batch->getPayload().getHandle() = {10, 20, 30, 40, 1, 2, 3, 4};

  auto R =
      G.createBatchedReduce("reduce.add", BatchedReduceNode::Mode::Add, batch);

  G.createSave("save", R, result);

  EE.compile(CompilationMode::Infer);

  EE.run({}, {});

  auto H = result->getPayload().getHandle();
  EXPECT_NEAR(H.at({0}), 11, 0.001);
  EXPECT_NEAR(H.at({1}), 22, 0.001);
  EXPECT_NEAR(H.at({2}), 33, 0.001);
  EXPECT_NEAR(H.at({3}), 44, 0.001);
}

TEST(Operator, batched_batched_add) {
  ExecutionEngine EE;

  auto &G = EE.getGraph();

  auto *batch = G.createVariable(ElemKind::FloatTy, {2, 3, 3}, "batch");
  auto *added = G.createVariable(ElemKind::FloatTy, {3, 3}, "added");
  auto *result = G.createVariable(ElemKind::FloatTy, {2, 3, 3}, "result");

  batch->getPayload().getHandle() = {9, 8, 7, 6, 5,  4,  3,  4,  5,
                                     6, 7, 8, 9, 10, 11, 12, 13, 14};
  added->getPayload().getHandle().clear(1.0);

  auto R = G.createBatchedArithmetic(
      "batch.add", BatchedArithmeticNode::Mode::Add, batch, added);
  G.createSave("save", R, result);

  EE.compile(CompilationMode::Infer);

  EE.run({}, {});

  auto H = result->getPayload().getHandle();
  EXPECT_NEAR(H.at({0, 0, 0}), 10, 0.001);
  EXPECT_NEAR(H.at({0, 0, 1}), 9, 0.001);
  EXPECT_NEAR(H.at({0, 0, 2}), 8, 0.001);
  EXPECT_NEAR(H.at({0, 1, 0}), 7, 0.001);
}