package com.example.staircrowdness;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import com.aliyun.tea.*;
import com.aliyun.iot20180120.models.QueryDevicePropertyStatusRequest;
import com.aliyun.iot20180120.models.QueryDevicePropertyStatusResponse;

import com.aliyun.teautil.models.RuntimeOptions;
import com.google.gson.Gson;


public class MainActivity extends AppCompatActivity {

    public static com.aliyun.iot20180120.Client createClient(String accessKeyId, String accessKeySecret) throws Exception {
        com.aliyun.teaopenapi.models.Config config = new com.aliyun.teaopenapi.models.Config()
                // 必填，您的 AccessKey ID
                .setAccessKeyId(accessKeyId)
                // 必填，您的 AccessKey Secret
                .setAccessKeySecret(accessKeySecret);
        // Endpoint 请参考 https://api.aliyun.com/product/Iot
        config.endpoint = "iot.cn-shanghai.aliyuncs.com";
        return new com.aliyun.iot20180120.Client(config);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        try {
            com.aliyun.iot20180120.Client client = createClient("LTAI5tJ1ANVSjv7cDzAHXS6b", "z8IJddHXqIDXqmWVcuzga9kvxzBgpJ");
            com.aliyun.iot20180120.models.QueryDevicePropertyStatusRequest queryDevicePropertyStatusRequest = new QueryDevicePropertyStatusRequest()
                    .setIotId("iot-06z00d3n6c4eo0p")
                    .setProductKey("k0ae3tXtQp0")
                    .setDeviceName("test1");
            QueryDevicePropertyStatusResponse response = client.queryDevicePropertyStatus(queryDevicePropertyStatusRequest);
            Log.d("sun",new Gson().toJson(response.body));
        } catch (Exception e) {
            e.printStackTrace();
        }



    }
}