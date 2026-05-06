// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/ashihmin_d_lab4_MLIR%shlibext --pass-pipeline="builtin.module(trace-conditions)" %s | FileCheck %s

module {
  // CHECK-LABEL: func @test_basic_if
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
      "test.else_op"() : () -> ()
    }
    return
  }

  // CHECK-LABEL: func @test_nested_if
  func.func @test_nested_if(%c1: i1, %c2: i1) {
    // CHECK: call @trace_condition_then_begin()
    scf.if %c1 {
      // CHECK: call @trace_condition_then_begin()
      scf.if %c2 {
        "inner.op"() : () -> ()
        // CHECK: call @trace_condition_then_end()
      }
      // CHECK: call @trace_condition_then_end()
    }
    return
  }

  // CHECK-LABEL: func @test_affine_no_else
  func.func @test_affine_no_else(%idx: index) {
    // CHECK: affine.if
    // CHECK-NEXT: call @trace_condition_then_begin()
    affine.if affine_set<(d0) : (d0 > 0)>(%idx) {
        "affine.op"() : () -> ()
        // CHECK: call @trace_condition_then_end()
    }
    // CHECK-NOT: call @trace_condition_else_begin()
    return
  }
}