// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/ashihmin_d_lab4_MLIR%shlibext --pass-pipeline="builtin.module(trace-conditions)" -allow-unregistered-dialect %s | FileCheck %s

// CHECK-LABEL: func.func @test_scf_if
func.func @test_scf_if(%cond: i1) {
  // CHECK: scf.if {{.*}} {
  // CHECK-NEXT: func.call @trace_condition_then_begin() : () -> ()
  // CHECK-NEXT: "test.op1"()
  // CHECK-NEXT: func.call @trace_condition_then_end() : () -> ()
  // CHECK-NEXT: } else {
  // CHECK-NEXT: func.call @trace_condition_else_begin() : () -> ()
  // CHECK-NEXT: "test.op2"()
  // CHECK-NEXT: func.call @trace_condition_else_end() : () -> ()
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
  // CHECK: scf.if {{.*}} {
  // CHECK-NEXT: func.call @trace_condition_then_begin() : () -> ()
  // CHECK-NEXT: func.call @trace_condition_then_end() : () -> ()
  // CHECK-NEXT: }
  scf.if %cond {
    scf.yield
  }
  return
}

#set = affine_set<(d0) : (d0 - 1 >= 0)>

// CHECK-LABEL: func.func @test_affine_if
func.func @test_affine_if(%idx: index) {
  // CHECK: affine.if {{.*}} {
  // CHECK-NEXT: func.call @trace_condition_then_begin() : () -> ()
  // CHECK-NEXT: "test.affine_op"()
  // CHECK-NEXT: func.call @trace_condition_then_end() : () -> ()
  // CHECK-NEXT: }
  affine.if #set(%idx) {
    "test.affine_op"() : () -> ()
  }
  return
}

// CHECK-DAG: func.func private @trace_condition_then_begin()
// CHECK-DAG: func.func private @trace_condition_then_end()
// CHECK-DAG: func.func private @trace_condition_else_begin()
// CHECK-DAG: func.func private @trace_condition_else_end()