:vsemantic position (VERTEX)
:vsemantic diffuse (COLOR COLOUR)

:shader simple_vs
(:type vertex
 :uniforms (
          :mvp worldviewproj_matrix)
 :program cg (
    :file Example_Basic.cg
    :entry simple_vs
    :profiles [vs_3_0 arbvp1 sce_vp_rsx])
 :program hlsl (
    :file Example_Basic.hlsl
    :entry simple_vs
    :profile vs_1_1))

:shader simple_ps
(:type pixel
 :program cg (
    :file Example_Basic.cg
    :entry simple_ps
    :profiles [ps_2_0 arbfp1 sce_fp_rsx])
 :program hlsl (
    :file Example_Basic.hlsl
    :entry simple_ps
    :profile ps_2_0))


