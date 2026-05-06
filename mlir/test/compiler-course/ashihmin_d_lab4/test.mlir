// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/ashihmin_d_lab4_MLIR%shlibext --pass-pipeline="builtin.module(trace-conditions)" -allow-unregistered-dialect %s | FileCheck %s

// CHECK-LABEL: func.func @test_basic_if
func.func @test_basic_if(%cond: i1) {
  // CHECK: scf.if %arg0 {
  // CHECK-NEXT: call @trace_condition_then_begin()
  // CHECK-NEXT: "test.op"()
  // CHECK-NEXT: call @trace_condition_then_end()
  // CHECK-NEXT: scf.yield
  scf.if %cond {
    "test.op"() : () -> ()
  } else {
    // CHECK: } else {
    // CHECK-NEXT: call @trace_condition_else_begin()
    // CHECK-NEXT: "test.else_op"()
    // CHECK-NEXT: call @trace_condition_else_end()
    // CHECK-NEXT: scf.yield
    "test.else_op"() : () -> ()
  }
  return
}

// CHECK-LABEL: func.func @test_no_else
func.func @test_no_else(%cond: i1) {
  // CHECK: scf.if %arg0 {
  // CHECK-NEXT: call @trace_condition_then_begin()
  // CHECK-NEXT: "test.op"()
  // CHECK-NEXT: call @trace_condition_then_end()
  scf.if %cond {
    "test.op"() : () -> ()
  }
  // CHECK-NOT: call @trace_condition_else_begin
  return
}

// CHECK-LABEL: func.func @test_nested
func.func @test_nested(%c1: i1, %c2: i1) {
  // CHECK: scf.if %arg0 {
  // CHECK-NEXT: call @trace_condition_then_begin()
  scf.if %c1 {
    // CHECK: scf.if %arg1 {
    // CHECK-NEXT: call @trace_condition_then_begin()
    scf.if %c2 {
      "inner"() : () -> ()
      // CHECK: call @trace_condition_then_end()
    }
    // CHECK: call @trace_condition_then_end()
  }
  return
}

// CHECK-LABEL: func.func @test_affine
func.func @test_affine(%idx: index) {
  // CHECK: affine.if
  // CHECK-NEXT: call @trace_condition_then_begin()
  affine.if affine_set<(d0) : (d0 - 1 >= 0)>(%idx) {
      "affine.op"() : () -> ()
      // CHECK: call @trace_condition_then_end()
  }
  return
}