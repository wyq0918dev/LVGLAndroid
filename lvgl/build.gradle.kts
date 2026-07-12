plugins {
    alias(libs.plugins.android.library)
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

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
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
            headers = "src/main/cpp/lvgl/include/lvgl"
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
}

dependencies {
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.core.ktx)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(libs.androidx.junit)
}