package lt.rokas.wirelessmic;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import lt.rokas.wirelessmic.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'wirelessmic' library on application startup.
    static {
        System.loadLibrary("wirelessmic");
    }

    private ActivityMainBinding binding;

    // Requesting permission to RECORD_AUDIO
    private boolean permissionToRecordAccepted = false;
    private final String [] permissions = {Manifest.permission.RECORD_AUDIO};
    private static final int REQUEST_RECORD_AUDIO_PERMISSION = 200;



    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        ActivityCompat.requestPermissions(this, permissions, REQUEST_RECORD_AUDIO_PERMISSION);

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
        binding.buttonStart.setOnClickListener((button)->startAudio());
        binding.buttonStop.setOnClickListener((button)->stopAudio());

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0) {
            switch (requestCode) {
                case REQUEST_RECORD_AUDIO_PERMISSION:
                    permissionToRecordAccepted = grantResults[0] == PackageManager.PERMISSION_GRANTED;
                    break;
            }
            if (!permissionToRecordAccepted) finish();
        }

    }


    /**
     * A native method that is implemented by the 'wirelessmic' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native void startAudio();
    public native void stopAudio();
}