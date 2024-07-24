; ModuleID = 'polly_example.bc'
source_filename = "polly_example.c"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx14.0.0"

@.str = private unnamed_addr constant [4 x i8] c"%f \00", align 1

; Function Attrs: nofree norecurse nosync nounwind ssp memory(argmem: readwrite) uwtable
define void @matrix_mul(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr nocapture noundef readonly %3, ptr nocapture noundef readonly %4, ptr nocapture noundef writeonly %5) local_unnamed_addr #0 {
  %7 = zext i32 %1 to i64
  %8 = zext i32 %2 to i64
  %9 = icmp sgt i32 %0, 0
  %10 = icmp sgt i32 %2, 0
  %11 = and i1 %9, %10
  br i1 %11, label %12, label %69

12:                                               ; preds = %6
  %13 = icmp sgt i32 %1, 0
  %14 = zext nneg i32 %0 to i64
  br i1 %13, label %15, label %20

15:                                               ; preds = %12
  %16 = and i64 %7, 1
  %17 = icmp eq i32 %1, 1
  %18 = and i64 %7, 2147483646
  %19 = icmp eq i64 %16, 0
  br label %23

20:                                               ; preds = %12
  %21 = shl nuw nsw i64 %14, 2
  %22 = mul i64 %21, %8
  tail call void @llvm.memset.p0.i64(ptr align 4 %5, i8 0, i64 %22, i1 false), !tbaa !5
  br label %69

23:                                               ; preds = %15, %66
  %24 = phi i64 [ %67, %66 ], [ 0, %15 ]
  %25 = mul nuw nsw i64 %24, %8
  %26 = getelementptr inbounds float, ptr %5, i64 %25
  %27 = mul nuw nsw i64 %24, %7
  %28 = getelementptr inbounds float, ptr %3, i64 %27
  br label %29

29:                                               ; preds = %63, %23
  %30 = phi i64 [ %64, %63 ], [ 0, %23 ]
  %31 = getelementptr inbounds float, ptr %26, i64 %30
  store float 0.000000e+00, ptr %31, align 4, !tbaa !5
  %32 = getelementptr inbounds float, ptr %4, i64 %30
  br i1 %17, label %53, label %.preheader

.preheader:                                       ; preds = %29
  br label %33

33:                                               ; preds = %.preheader, %33
  %34 = phi i64 [ %50, %33 ], [ 0, %.preheader ]
  %35 = phi float [ %49, %33 ], [ 0.000000e+00, %.preheader ]
  %36 = phi i64 [ %51, %33 ], [ 0, %.preheader ]
  %37 = getelementptr inbounds float, ptr %28, i64 %34
  %38 = load float, ptr %37, align 4, !tbaa !5
  %39 = mul nuw nsw i64 %34, %8
  %40 = getelementptr inbounds float, ptr %32, i64 %39
  %41 = load float, ptr %40, align 4, !tbaa !5
  %42 = tail call float @llvm.fmuladd.f32(float %38, float %41, float %35)
  store float %42, ptr %31, align 4, !tbaa !5
  %43 = or disjoint i64 %34, 1
  %44 = getelementptr inbounds float, ptr %28, i64 %43
  %45 = load float, ptr %44, align 4, !tbaa !5
  %46 = mul nuw nsw i64 %43, %8
  %47 = getelementptr inbounds float, ptr %32, i64 %46
  %48 = load float, ptr %47, align 4, !tbaa !5
  %49 = tail call float @llvm.fmuladd.f32(float %45, float %48, float %42)
  store float %49, ptr %31, align 4, !tbaa !5
  %50 = add nuw nsw i64 %34, 2
  %51 = add i64 %36, 2
  %52 = icmp eq i64 %51, %18
  br i1 %52, label %.loopexit, label %33, !llvm.loop !9

.loopexit:                                        ; preds = %33
  br label %53

53:                                               ; preds = %.loopexit, %29
  %54 = phi i64 [ 0, %29 ], [ %50, %.loopexit ]
  %55 = phi float [ 0.000000e+00, %29 ], [ %49, %.loopexit ]
  br i1 %19, label %63, label %56

56:                                               ; preds = %53
  %57 = getelementptr inbounds float, ptr %28, i64 %54
  %58 = load float, ptr %57, align 4, !tbaa !5
  %59 = mul nuw nsw i64 %54, %8
  %60 = getelementptr inbounds float, ptr %32, i64 %59
  %61 = load float, ptr %60, align 4, !tbaa !5
  %62 = tail call float @llvm.fmuladd.f32(float %58, float %61, float %55)
  store float %62, ptr %31, align 4, !tbaa !5
  br label %63

63:                                               ; preds = %53, %56
  %64 = add nuw nsw i64 %30, 1
  %65 = icmp eq i64 %64, %8
  br i1 %65, label %66, label %29, !llvm.loop !11

66:                                               ; preds = %63
  %67 = add nuw nsw i64 %24, 1
  %68 = icmp eq i64 %67, %14
  br i1 %68, label %.loopexit1, label %23, !llvm.loop !12

.loopexit1:                                       ; preds = %66
  br label %69

69:                                               ; preds = %.loopexit1, %20, %6
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fmuladd.f32(float, float, float) #1

; Function Attrs: nofree nounwind ssp uwtable
define noundef i32 @main() local_unnamed_addr #2 {
  %1 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 3.000000e+01)
  %2 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 2.400000e+01)
  %3 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 1.800000e+01)
  %4 = tail call i32 @putchar(i32 10)
  %5 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 8.400000e+01)
  %6 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 6.900000e+01)
  %7 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 5.400000e+01)
  %8 = tail call i32 @putchar(i32 10)
  %9 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 1.380000e+02)
  %10 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 1.140000e+02)
  %11 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, double noundef 9.000000e+01)
  %12 = tail call i32 @putchar(i32 10)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #3

; Function Attrs: nofree nounwind
declare noundef i32 @putchar(i32 noundef) local_unnamed_addr #4

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg) #5

attributes #0 = { nofree norecurse nosync nounwind ssp memory(argmem: readwrite) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { nofree nounwind ssp uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #3 = { nofree nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cmov,+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "tune-cpu"="generic" }
attributes #4 = { nofree nounwind }
attributes #5 = { nocallback nofree nounwind willreturn memory(argmem: write) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"Homebrew clang version 18.1.7"}
!5 = !{!6, !6, i64 0}
!6 = !{!"float", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
!11 = distinct !{!11, !10}
!12 = distinct !{!12, !10}
