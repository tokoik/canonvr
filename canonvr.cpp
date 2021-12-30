//
// ゲームグラフィックス特論宿題アプリケーション
//
#include "GgApp.h"

// プロジェクト名
#ifndef PROJECT_NAME
#  define PROJECT_NAME "canonvr"
#endif

// OpenCV によるビデオキャプチャ
#include "CamCv.h"

// ファイルダイアログ
#include "nfd.h"

// 背景画像の展開に使用するバーテックスシェーダのソースファイル名
const char* const capture_vsrc{ "canonvr.vert" };

// 背景画像の展開に使用するフラグメントシェーダのソースファイル名
const char* const capture_fsrc{ "texture.frag" };

// 背景画像の描画に用いるメッシュの格子点数
constexpr int screen_samples{ 1271 };

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // ウィンドウを作成する
  Window window{ PROJECT_NAME };

  // ウィンドウが開けたかどうか確かめる
  if (!window.get())
  {
    // ウィンドウが開けなかった
    throw std::runtime_error("Can't open GLFW window.");
  }

  // 背景画像の取得に使用するカメラの解像度 (0 ならカメラから取得)
  int capture_width{ 0 };
  int capture_height{ 0 };

  // 背景画像の取得に使用するカメラのフレームレート (0 ならカメラから取得)
  int capture_fps{ 0 };

  // 背景描画用のシェーダプログラムを読み込む
  const GLuint expansion(ggLoadShader(capture_vsrc, capture_fsrc));
  if (!expansion)
  {
    // シェーダが読み込めなかった
    throw std::runtime_error("Can't create program object.");
  }

  // uniform 変数の場所を指定する
  const GLint gapLoc{ glGetUniformLocation(expansion, "gap") };
  const GLint screenLoc{ glGetUniformLocation(expansion, "screen") };
  const GLint focalLoc{ glGetUniformLocation(expansion, "focal") };
  const GLint rotationLoc{ glGetUniformLocation(expansion, "rotation") };
  const GLint circleLoc{ glGetUniformLocation(expansion, "circle") };
  const GLint imageLoc{ glGetUniformLocation(expansion, "image") };

  // 背景描画のためのメッシュを作成する
  //   頂点座標値を vertex shader で生成するので VBO は必要ない
  const GLuint mesh{ []() { GLuint mesh; glGenVertexArrays(1, &mesh); return mesh; } () };

  // 背景画像のファイル名
  std::string file;

  // 背景画像のテクスチャ
  GLuint image{ 0 };

  // ラジアン変換
  constexpr GLfloat toRad{ 0.00872664626f };

  // 背景画像のイメージサークルの半径
  float radius_x{ 180.0f };
  float radius_y{ 180.0f };

  // 背景画像のイメージサークルの中心
  float center_x{ 0.0f };
  float center_y{ 0.0f };

  // 背景画像のイメージサークルの間隔
  float offset_x{ 0.0f };
  float offset_y{ 0.0f };

#ifdef IMGUI_VERSION
  //
  // ImGui の初期設定
  //

  // ImGui のコンテキストの設定と入出力のデータへの参照を得る
  ImGuiIO& io = ImGui::GetIO();
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  ImFont* font = io.Fonts->AddFontFromFileTTF("Mplus1-Regular.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  IM_ASSERT(font != NULL);

  // Native File Dialog Extended の初期化
  NFD_Init();

  // メニューを表示する
  bool showMenu{ true };

#endif

  // カメラ入力
  CamCv camera;

  // 描くものがない時の背景色
  glClearColor(0.2f, 0.3f, 0.6f, 1.0f);

  // 隠面消去を行わない
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // ウィンドウが開いている間繰り返す
  while (window)
  {
#ifdef IMGUI_VERSION
    //
    // ImGui によるユーザインタフェース
    //

    // ImGui のフレームを準備する
    ImGui::NewFrame();

    // スペースキーがタイプされたらメニューの表示／非表示を切り替える
    if (window.getLastKey() == ' ') showMenu = !showMenu;

    // メニュー表示
    if (showMenu)
    {
      // 光源位置を決定する
      ImGui::SetNextWindowPos(ImVec2(4, 4), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(384, 232), ImGuiCond_Once);
      ImGui::Begin(u8"コントロールパネル");
      if (ImGui::Button(u8"開く"))
      {
        // ファイルダイアログから得るパス
        nfdchar_t* filepath{ NULL };

        // ファイルダイアログを開く
        const nfdfilteritem_t filter[]{ "Movies", "mp4,mpv,avi,mov,mkv" };
        if (NFD_OpenDialog(&filepath, filter, 1, NULL) == NFD_OKAY)
        {
          // キャプチャを停止する
          camera.stop();

          // カメラを閉じる
          camera.close();

          // カメラの使用を開始する
          if (camera.open(filepath, capture_width, capture_height, capture_fps))
          {
            // 切り替えたデバイスの特性を取り出す
            capture_width = camera.getWidth();
            capture_height = camera.getHeight();
            capture_fps = static_cast<int>(camera.getFps());

            // 背景用のテクスチャを作成する
            //   ポリゴンでビューポート全体を埋めるので背景は表示されない。
            //   GL_CLAMP_TO_BORDER にしておけばテクスチャの外が GL_TEXTURE_BORDER_COLOR になるので、これが背景色になる。
            glDeleteTextures(1, &image);
            glGenTextures(1, &image);
            glBindTexture(GL_TEXTURE_2D, image);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, capture_width, capture_height, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            // 背景色は表示されないが合成時に 0 にしておく必要がある
            constexpr GLfloat background[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, background);

            // ファイルパスを保存しておく
            file = filepath;

            // ファイルパスの取り出しに使ったメモリを開放する
            if (filepath) NFD_FreePath(filepath);

            // キャプチャを開始する
            camera.start();
          }
          else
          {
            image = 0;
          }
        }
      }
      ImGui::SameLine();
      ImGui::Text("File: %s", file.c_str());

      ImGui::DragFloat(u8"水平画角", &radius_x, 0.1f, -360.0f, 360.0f, "%.2f");
      ImGui::DragFloat(u8"垂直画角", &radius_y, 0.1f, -360.0f, 360.0f, "%.2f");
      ImGui::DragFloat(u8"水平位置", &center_x, 0.01f, -1.0f, 1.0f, "%.2f");
      ImGui::DragFloat(u8"垂直位置", &center_y, 0.01f, -1.0f, 1.0f, "%.2f");
      ImGui::DragFloat(u8"水平間隔", &offset_x, 0.01f, -1.0f, 1.0f, "%.2f");
      ImGui::DragFloat(u8"垂直間隔", &offset_y, 0.01f, -1.0f, 1.0f, "%.2f");
      ImGui::End();
    }

    // ImGui のフレームに描画する
    ImGui::Render();
#endif

    // テクスチャが有効なら
    if (image > 0)
    {
      // 背景画像の展開に用いるシェーダプログラムの使用を開始する
      glUseProgram(expansion);

      // スクリーンを縦に２分割する
      const GLsizei width{ window.getWidth() / 2 };
      const GLsizei height{ window.getHeight() };
      const GLfloat aspect{ window.getAspect() * 0.5f };

      // スクリーンの矩形の格子点数
      //   標本点の数 (頂点数) n = x * y とするとき、これにアスペクト比 a = x / y をかければ、
      //   a * n = x * x となるから x = sqrt(a * n), y = n / x; で求められる。
      //   この方法は頂点属性を持っていないので実行中に標本点の数やアスペクト比の変更が容易。
      const GLsizei slices(static_cast<GLsizei>(sqrt(screen_samples * aspect)));
      const GLsizei stacks(screen_samples / slices - 1); // 描画するインスタンスの数なので先に 1 を引いておく。

      // スクリーンの格子間隔
      //   クリッピング空間全体を埋める四角形は [-1, 1] の範囲すなわち縦横 2 の大きさだから、
      //   それを縦横の (格子数 - 1) で割って格子の間隔を求める。
      glUniform2f(gapLoc, 2.0f / (slices - 1), 2.0f / stacks);

      // スクリーンのサイズと中心位置
      //   screen[0] = (right - left) / 2
      //   screen[1] = (top - bottom) / 2
      //   screen[2] = (right + left) / 2
      //   screen[3] = (top + bottom) / 2
      const GLfloat screen[] = { aspect, 1.0f, 0.0f, 0.0f };
      glUniform4fv(screenLoc, 1, screen);

      // スクリーンまでの焦点距離
      //   window.getWheel() は [-100, 49] の範囲を返す。
      //   したがって焦点距離 focal は [1 / 3, 1] の範囲になる。
      //   これは焦点距離が長くなるにしたがって変化が大きくなる。
      glUniform1f(focalLoc, -50.0f / (window.getWheelY() - 50.0f));

      // 背景に対する視線の回転行列
      glUniformMatrix4fv(rotationLoc, 1, GL_TRUE, window.getRotationMatrix().get());

      // テクスチャユニットを指定する
      glUniform1i(imageLoc, 0);

      // キャプチャした画像を背景用のテクスチャに転送する
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, image);
      camera.transmit();

      // メッシュを指定する
      glBindVertexArray(mesh);

      // 画角
      const GLfloat rx{ radius_x * toRad };
      const GLfloat ry{ radius_y * toRad };

      // 位置
      const GLfloat xl{ center_x - 0.25f - offset_x };
      const GLfloat xr{ center_x + 0.25f + offset_x };

      // 左目の映像を描画する
      glUniform4f(circleLoc, rx, ry, xl, center_y + offset_y);
      glViewport(0, 0, width, height);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks);

      // 右目の映像を描画する
      glUniform4f(circleLoc, rx, ry, xr, center_y - offset_y);
      glViewport(width, 0, width, height);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks);
    }
    else
    {
      // 描くものがないので画面クリアだけしておく
      glClear(GL_COLOR_BUFFER_BIT);
    }

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
