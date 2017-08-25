package example.com.br.detectcardnumber;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import butterknife.BindView;
import butterknife.ButterKnife;

public class FirstActivity extends AppCompatActivity {

    @BindView(R.id.btn_camera_preview)
    Button btnCameraPreview;

    @BindView(R.id.btn_focus_score)
    Button btnFocusScore;

    @BindView(R.id.btn_detect_edges)
    Button btnDetectEdges;

    @BindView(R.id.btn_transform_3d)
    Button btnTransf3d;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.first_activity);

        ButterKnife.bind(this);

        btnCameraPreview.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(FirstActivity.this, CameraPreview.class);
                startActivity(intent);
            }
        });

        btnFocusScore.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(FirstActivity.this, FocusScore.class);
                startActivity(intent);
            }
        });

        btnDetectEdges.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(FirstActivity.this, DetectEdges.class);
                startActivity(intent);
            }
        });

        btnTransf3d.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(FirstActivity.this, Transform3D.class);
                startActivity(intent);
            }
        });
    }

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }
}
