package com.wyq0918dev.lvgl.demo

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.twotone.Info
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.BottomAppBar
import androidx.compose.material3.Button
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.FloatingActionButtonDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import com.wyq0918dev.lvgl.demo.ui.theme.LVGLAndroidTheme
import com.wyq0918dev.lvgl.demo.ui.view.LVGLDemoView

class MainActivity : ComponentActivity() {

    @OptIn(ExperimentalMaterial3Api::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val infoExpanded = remember { mutableStateOf(value = false) }
            LVGLAndroidTheme {
                if (infoExpanded.value) AlertDialog(
                    onDismissRequest = { infoExpanded.value = false },
                    title = {
                        Text(text = "Info")
                    },
                    text = {
                        Text(text = stringResource(id = R.string.info_text))
                    },
                    confirmButton = {
                        Button(
                            onClick = { infoExpanded.value = false },
                        ) {
                            Text(text = "确定")
                        }
                    },
                )
                Scaffold(
                    modifier = Modifier.fillMaxSize(),
                    topBar = {
                        TopAppBar(
                            title = {
                                Text(text = "LVGL Android Demo")
                            }
                        )
                    },
                    bottomBar = {
                        BottomAppBar(
                            floatingActionButton = {
                                FloatingActionButton(
                                    onClick = { infoExpanded.value = true },
                                    elevation = FloatingActionButtonDefaults.bottomAppBarFabElevation(),
                                ) {
                                    Icon(
                                        imageVector = Icons.TwoTone.Info,
                                        contentDescription = null,
                                    )
                                }
                            },
                            actions = {

                            }
                        )
                    }
                ) { innerPadding ->
                    BoxWithConstraints(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(paddingValues = innerPadding)
                            .padding(all = 16.dp),
                        contentAlignment = Alignment.Center,
                    ) {
                        val aspectRatio = 480f / 320f
                        val (width, height) = if (maxWidth / maxHeight > aspectRatio) {
                            maxHeight * aspectRatio to maxHeight
                        } else {
                            maxWidth to maxWidth / aspectRatio
                        }
                        Surface(
                            modifier = Modifier.size(width, height),
                            color = MaterialTheme.colorScheme.primaryContainer,
                            shape = MaterialTheme.shapes.large,
                        ) {
                            AndroidView(
                                factory = { context -> LVGLDemoView(context) },
                                modifier = Modifier.fillMaxSize(),
                            )
                        }
                    }
                }
            }
        }
    }
}