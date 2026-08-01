plugins {
    id("com.android.application")
}

android {
    namespace = "cz.mormegil.vrvideoplayer"
    compileSdk = 37

    defaultConfig {
        applicationId = "cz.mormegil.vrvideoplayer"
        minSdk = 26
        targetSdk = 37
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters.add("armeabi-v7a")
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    buildFeatures {
        viewBinding = true
    }
    ndkVersion = "29.0.14206865"
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(21)
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.19.0")
    implementation("androidx.appcompat:appcompat:1.7.1")
    implementation("com.google.android.material:material:1.14.0")
    implementation("androidx.constraintlayout:constraintlayout:2.2.2")
    implementation("com.google.cardboard.sdk:sdk:1.34.0@aar")

    // Cardboard SDK dependencies:
    // Android Mobile Vision
    // TODO(b/217176538) Migrate to ML Kit.
    implementation("com.google.android.gms:play-services-vision:20.1.3")
    implementation("com.google.protobuf:protobuf-javalite:4.35.1")

    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.3.0")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.7.0")
}
