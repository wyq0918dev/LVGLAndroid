plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.maven.publish)
}

android {
    namespace = "com.wyq0918dev.lvgl"
    compileSdk {
        version = release(version = 37)
    }
    defaultConfig {
        minSdk {
            version = release(version = 26)
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
        }
    }
    buildFeatures {
        prefabPublishing = true
    }
    prefab {
        create("lvgl") {
            headers = "src/main/cpp/lvgl"
//            headers = "src/main/cpp/lvgl/include/lvgl"
        }
        create("lvgl_demos") {
            headers = "src/main/cpp/lvgl/demos"
        }
        create("lvgl_examples") {
            headers = "src/main/cpp/lvgl/examples"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
    publishing {
        singleVariant("release") {
            withSourcesJar()
            withJavadocJar()
        }
    }
}

publishing {
    publications {
        create<MavenPublication>("release") {
            groupId = "com.github.wyq0918dev"
            artifactId = "LVGLAndroid"
            version = "9.5.0"
            afterEvaluate {
                from(components["release"])
            }
            pom {
                name = "LVGL"
                description = "LVGL Prefab library for Android"
                url = "https://github.com/wyq0918dev/LVGLAndroid"
                licenses {
                    license {
                        name = "MIT"
                        url = "https://github.com/wyq0918dev/LVGLAndroid/blob/master/LICENSE"
                    }
                }
                developers {
                    developer {
                        name = "wyq0918dev"
                        url = "https://github.com/wyq0918dev"
                    }
                }
                scm {
                    connection = "scm:git:https://github.com/wyq0918dev/LVGLAndroid.git"
                    url = "https://github.com/wyq0918dev/LVGLAndroid"
                }
            }
        }
    }
}