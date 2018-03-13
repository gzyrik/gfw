using System;
using System.Runtime.InteropServices;
class ftyuv
{
    public enum flags{
        FTYUV_LEFT          = 1<<0,     /* layout default */
        FTYUV_RIGHT         = 1<<1,
        FTYUV_TOP           = 1<<2,     /* layout default */
        FTYUV_BOTTOM        = 1<<3,
        FTYUV_HCENTER       = FTYUV_LEFT | FTYUV_RIGHT,
        FTYUV_VCENTER       = FTYUV_TOP | FTYUV_BOTTOM,
        FTYUV_CENTER        = FTYUV_HCENTER | FTYUV_VCENTER,

        FTYUV_LINE_LEFT     = 1<<4,     /* line default */
        FTYUV_LINE_RIGHT    = 1<<5,
        FTYUV_LINE_TOP      = 1<<6,     /* line default */
        FTYUV_LINE_BOTTOM   = 1<<7,
        FTYUV_LINE_HCENTER  = FTYUV_LINE_LEFT|FTYUV_LINE_RIGHT,
        FTYUV_LINE_VCENTER  = FTYUV_LINE_TOP|FTYUV_LINE_BOTTOM,

        /* default is HORIZONTAL */
        FTYUV_VERTICAL      = 1<<8,     

        /* default is y-plane xor */
        FTYUV_BLEND         = 1<<9,
        FTYUV_STROKE        = 1<<10,

        /* defualt is plane */
        FTYUV_PACKED        = 1<<11,

        /* default is yuv444 */
        FTYUV_420           = 1<<12,
        FTYUV_422           = 1<<13,
        FTYUV_411           = 1<<14
    };

    [DllImport("ftyuv.dll", CharSet = CharSet.Ansi)]
    public static extern IntPtr ftyuv_new(string        fontfile,
                                          int           ptsize);

    [DllImport("ftyuv.dll")]
    public static extern void ftyuv_unref (IntPtr       ftyuv);

    [DllImport("ftyuv.dll")]
    public static extern void ftyuv_space (IntPtr       ftyuv,
                                           int          hsapce,
                                           int          vspace);

    [DllImport("ftyuv.dll")]
    public static extern void ftyuv_margin (IntPtr      ftyuv,
                                            int         hmargin, 
                                            int         vmargin);

    [DllImport("ftyuv.dll")]
    public static extern void ftyuv_color (IntPtr       ftyuv,
                                           uint         font_xrgb,
                                           uint         border_xrgb);

    [DllImport("ftyuv.dll", CharSet = CharSet.Unicode)]
    public static extern void ftyuv_draw (IntPtr        ftyuv,
                                          string        str, 
                                          int           len,
                                          ref byte      i420,
                                          int           width,
                                          int           height,
                                          uint          flags);
};
