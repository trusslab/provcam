echo "packing all vcu_gst_app files"
cd vcu_apm_lib;
zip -rq ../vcu_apm_lib.zip *;
cd ../vcu_gst_lib;
zip -rq ../vcu_gst_lib.zip *;
cd ../vcu_gst_app;
zip -rq ../vcu_gst_app.zip *;
cd ../vcu_video_lib;
zip -rq ../vcu_video_lib.zip *;
