// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/ashihmin_d_lab4_MLIR%shlibext --pass-pipeline="builtin.module(trace-conditions)" -allow-unregistered-dialect %s | FileCheck %s

// CHECK-LABEL: func.func @test_scf_if
func.func @test_scf_if(%cond: i1) {
  // CHECK: scf.if %arg0 {
  // CHECK-NEXT: call @trace_condition_then_begin()
  // CHECK-NEXT: "test.op1"()
  // CHECK-NEXT: call @trace_condition_then_end()
  // CHECK-NEXT: scf.yield
  // CHECK-NEXT: } else {
  // CHECK-NEXT: call @trace_condition_else_begin()
  // CHECK-NEXT: "test.op2"()
  // CHECK-NEXT: call @trace_condition_else_end()
  scf.if %cond {
    "test.op1"() : () -> ()
    scf.yield
  } else {
    "test.op2"() : () -> ()
    scf.yield
  }
  return
}

// CHECK-LABEL: func.func @test_empty_then
func.func @test_empty_then(%cond: i1) {
  // CHECK: scf.if %arg0 {
  // CHECK-NEXT: call @trace_condition_then_begin()
  // CHECK-NEXT: call @trace_condition_then_end()
  // CHECK-NEXT: scf.yield
  scf.if %cond {
    scf.yield
  }
  return
}

// CHECK-LABEL: func.func @test_affine_if
func.func @test_affine_if(%idx: index) {
  // CHECK: affine.if
  // CHECK-NEXT: call @trace_condition_then_begin()
  affine.if affine_set<(d0) : (d0 - 1 >= 0)>(%idx) {
    "test.affine_op"() : () -> ()
  }
  // CHECK: call @trace_condition_then_end()
  return
}

// CHECK: func.func private @trace_condition_then_begin()
// CHECK: func.func private @trace_condition_then_end()