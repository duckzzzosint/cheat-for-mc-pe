package com.toker.mccheat;

import android.app.*;
import android.content.Intent;
import android.os.IBinder;

public class InjectorService extends Service {
    private static final String CH = "mc_cheat";

    @Override public void onCreate() {
        super.onCreate();
        NotificationChannel ch = new NotificationChannel(
            CH, "MC Cheat", NotificationManager.IMPORTANCE_LOW);
        getSystemService(NotificationManager.class).createNotificationChannel(ch);

        startForeground(1, new Notification.Builder(this, CH)
            .setContentTitle("MC Cheat")
            .setContentText("Active")
            .setSmallIcon(android.R.drawable.ic_menu_compass)
            .build());
    }

    @Override public int     onStartCommand(Intent i, int f, int s) { return START_STICKY; }
    @Override public IBinder onBind(Intent i) { return null; }
}
