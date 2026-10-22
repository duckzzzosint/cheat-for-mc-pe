// Language: Java 17 | Android 26+ | Requires root
package com.toker.mccheat;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.method.ScrollingMovementMethod;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import java.io.*;
import java.util.concurrent.TimeUnit;

public class MainActivity extends AppCompatActivity {

    private static final String MC_PKG   = "com.mojang.minecraftpe";
    private static final String TMP      = "/data/local/tmp/";
    private static final String LIB_NAME = "libcheat.so";
    private static final String INJ_NAME = "mc_injector";

    private TextView     tvStatus, tvLog;
    private EditText     etPid, etLibPath;
    private StringBuilder log = new StringBuilder();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvStatus  = findViewById(R.id.tv_status);
        tvLog     = findViewById(R.id.tv_log);
        etPid     = findViewById(R.id.et_pid);
        etLibPath = findViewById(R.id.et_lib_path);

        tvLog.setMovementMethod(new ScrollingMovementMethod());
        requestOverlay();

        findViewById(R.id.btn_extract).setOnClickListener(v -> extractAssets());
        findViewById(R.id.btn_launch) .setOnClickListener(v -> launchMinecraft());
        findViewById(R.id.btn_inject) .setOnClickListener(v -> doInject());
    }

    // ── Overlay permission ─────────────────────────────────────────────────

    private void requestOverlay() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && !Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
        }
    }

    // ── Extract mc_injector + libcheat.so from APK assets ─────────────────

    private void extractAssets() {
        status("Extracting...", "#ffaa00");
        new Thread(() -> {
            try {
                extract(LIB_NAME, TMP + LIB_NAME);
                extract(INJ_NAME, TMP + INJ_NAME);
                su("chmod 755 " + TMP + INJ_NAME);
                su("chmod 644 " + TMP + LIB_NAME);
                runOnUiThread(() -> {
                    status("Extracted", "#44cc44");
                    log("OK " + TMP + INJ_NAME);
                    log("OK " + TMP + LIB_NAME);
                });
            } catch (Exception e) {
                runOnUiThread(() -> {
                    status("Failed", "#ff4444");
                    log("ERR " + e.getMessage());
                });
            }
        }).start();
    }

    private void extract(String asset, String dest) throws IOException {
        try (InputStream in   = getAssets().open(asset);
             OutputStream out = new FileOutputStream(dest)) {
            byte[] buf = new byte[8192]; int n;
            while ((n = in.read(buf)) != -1) out.write(buf, 0, n);
        }
    }

    // ── Launch Minecraft ───────────────────────────────────────────────────

    private void launchMinecraft() {
        Intent i = getPackageManager().getLaunchIntentForPackage(MC_PKG);
        if (i == null) { log("Minecraft not found (" + MC_PKG + ")"); return; }
        startActivity(i);
        log("Launched — wait for title screen, then inject.");
    }

    // ── Inject ─────────────────────────────────────────────────────────────

    private void doInject() {
        String pidText = etPid.getText().toString().trim();
        String libPath = etLibPath.getText().toString().trim();
        status("Injecting...", "#ffaa00");

        new Thread(() -> {
            try {
                String pid = pidText.isEmpty() ? findPid() : pidText;
                if (pid == null) {
                    runOnUiThread(() -> {
                        log("ERR: Minecraft not running");
                        status("Failed", "#ff4444");
                    });
                    return;
                }
                runOnUiThread(() -> log("PID: " + pid));

                String cmd    = TMP + INJ_NAME + " " + pid + " " + libPath;
                String output = su(cmd);

                runOnUiThread(() -> {
                    log(output.trim());
                    if (output.contains("Injection OK")) status("Injected", "#44cc44");
                    else                                  status("Done (check log)", "#ffaa00");
                });
            } catch (Exception e) {
                runOnUiThread(() -> {
                    log("ERR " + e.getMessage());
                    status("Failed", "#ff4444");
                });
            }
        }).start();
    }

    // ── Helpers ────────────────────────────────────────────────────────────

    private String findPid() {
        File proc = new File("/proc");
        File[] dirs = proc.listFiles(f -> f.getName().matches("\\d+"));
        if (dirs == null) return null;
        for (File d : dirs) {
            try (FileInputStream fis = new FileInputStream(new File(d, "cmdline"))) {
                byte[] b = new byte[256]; int n = fis.read(b);
                String cmd = new String(b, 0, n).replace('\0', ' ').trim();
                if (cmd.startsWith(MC_PKG)) return d.getName();
            } catch (Exception ignored) {}
        }
        return null;
    }

    private String su(String cmd) throws IOException, InterruptedException {
        Process p = Runtime.getRuntime().exec("su");
        try (DataOutputStream dos = new DataOutputStream(p.getOutputStream())) {
            dos.writeBytes(cmd + "\n");
            dos.writeBytes("exit\n");
            dos.flush();
        }
        StringBuilder sb = new StringBuilder();
        byte[] buf = new byte[2048];
        long deadline = System.currentTimeMillis() + 8000;
        while (System.currentTimeMillis() < deadline) {
            if (p.getInputStream().available() > 0) {
                int n = p.getInputStream().read(buf);
                sb.append(new String(buf, 0, n));
            }
            if (p.getErrorStream().available() > 0) {
                int n = p.getErrorStream().read(buf);
                sb.append(new String(buf, 0, n));
            }
            if (p.waitFor(50, TimeUnit.MILLISECONDS)) break;
        }
        return sb.toString();
    }

    private void log(String msg) {
        runOnUiThread(() -> {
            log.append(msg).append('\n');
            tvLog.setText(log.toString());
        });
    }

    private void status(String msg, String hexColor) {
        runOnUiThread(() -> tvStatus.setText("● " + msg));
    }
}
