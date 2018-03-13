:shader sky_box_vs
(:type vertex
 :uniforms (
          :mvp worldviewproj_matrix)
 :program cg (
    :file sky.cg
    :entry sky_box_vs
    :profiles [vs_3_0 arbvp1 sce_vp_rsx])
 :program hlsl (
    :file sky.hlsl
    :entry sky_box_vs
    :profile vs_1_1))

:shader sky_box_ps
(:type pixel
 :program cg (
    :file sky.cg
    :entry sky_box_ps
    :profiles [ps_2_0 arbfp1 sce_fp_rsx])
 :program hlsl (
    :file sky.hlsl
    :entry sky_box_ps
    :profile ps_2_0))


