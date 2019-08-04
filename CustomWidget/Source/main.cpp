#include "Ange.h"
#include "platform.h"

#include <thread>
#include <stdio.h>
#include <cwchar>
#include <string.h>

using namespace Ange;


#define CI_BG			1001
#define CI_LINE			1002
#define CI_TEXT			1003
#define CI_TEXT_HEX		1004


class ColorInfo : public CustomWidget
{
public:

	ColorInfo(Window* window, Widget2DProps props, Font* font) : CustomWidget(window, props)
	{
		//Create Background
		m_Components.insert(std::make_pair(
			CI_BG,
			new Background(window, props, BackgroundTheme{ Color(0,0,0,255),Color{0,0,0,0}, {0,0} })
		));

		//Create Line
		props.Position = { props.Position.tX + 20, props.Position.tY + (int)props.Dimensions.tHeight / 2 };
		props.Dimensions = { props.Dimensions.tWidth - 40, 1 };
		m_Components.insert(std::make_pair(
			CI_LINE,
			new Background(window, props, BackgroundTheme{ Color(255,255,255,255),Color{0,0,0,0}, {0,0} })
		));

		//Add Text
		props.Position += {0, 4};
		props.Dimensions = { props.Dimensions.tWidth, (size_t)font->GetLineHeight(10) };
		m_Components.insert(std::make_pair(
			CI_TEXT,
			new Text(window, props, { 10, Color{255,255,255,255}, font}, L"rgb(0,0,0)")
		));

		props.Position += {0, -8-font->GetLineHeight(10)};
		m_Components.insert(std::make_pair(
			CI_TEXT_HEX,
			new Text(window, props, { 10, Color{255,255,255,255}, font }, L"#000000")
		));

	}

	void SetColor(Color color)
	{
		//Change background color
		auto bg = (Background*)GetComponent(CI_BG);
		bg->SetColor(color);

		//Change line color
		auto line = (Background*)GetComponent(CI_LINE);
		auto text = (Text*)GetComponent(CI_TEXT);
		auto textHex = (Text*)GetComponent(CI_TEXT_HEX);

		if (color.GetBrightness() < 128)
		{
			line->SetColor(Color{255,255,255,255});
			text->SetColor(Color{ 255,255,255,255 });
			textHex->SetColor(Color{ 255,255,255,255 });

		} else {
			line->SetColor(Color{ 0,0,0,255 });
			text->SetColor(Color{ 0,0,0,255 });
			textHex->SetColor(Color{ 0,0,0,255 });
		}

		wchar_t buffer[17];
		swprintf(buffer, 17, L"rgb(%i,%i,%i)", (int)(color.r*255), (int)(color.g*255), (int)(color.b*255));
		text->SetText(std::wstring(buffer));
		
		swprintf(buffer, 17, L"#%02x%02x%02x", (int)(color.r * 255), (int)(color.g * 255), (int)(color.b * 255));
		textHex->SetText(std::wstring(buffer));
	}
};


class ColorPicker : public Task<Nothing, Nothing>
{
public:

	ColorPicker() : Task()
	{	
		m_Picked = false;
		m_Drag = false;

		Run();
		Join(false);
	}

	~ColorPicker()
	{
	}

	bool IfPicked()
	{
		return m_Picked;
	}

	Color GetColor()
	{
		return Color((int)m_Pixel[0], m_Pixel[1], m_Pixel[2], 255);
	}

private:

	bool PreciseClickHandler(Event* ev)
	{
		Point<int> pos;
		bool change = false;
		if (ev->GetEventType() == EventType::MouseClick) {
			MouseClickEvent* mce = (MouseClickEvent*)ev;
			pos = mce->GetPosition();
			change = true;
		} else if (ev->GetEventType() == EventType::MouseMove) {
			MouseMoveEvent* mme = (MouseMoveEvent*)ev;
			if (m_PrecisePalete->GetState() == 2 || (m_PrecisePalete->GetState() == 1 && m_PrecisePalete->GetDragInfo() == true)) {
				pos = mme->GetPosition();
				if (pos.tX < m_PrecisePalete->GetPosition().tX) pos.tX = m_PrecisePalete->GetPosition().tX;
				if (pos.tY < m_PrecisePalete->GetPosition().tY) pos.tY = m_PrecisePalete->GetPosition().tY;
				if (pos.tX > m_PrecisePalete->GetPosition().tX + m_PrecisePalete->GetDimension().tWidth) pos.tX = m_PrecisePalete->GetPosition().tX + m_PrecisePalete->GetDimension().tWidth - 2;
				if (pos.tY > m_PrecisePalete->GetPosition().tY + m_PrecisePalete->GetDimension().tHeight) pos.tY = m_PrecisePalete->GetPosition().tY + m_PrecisePalete->GetDimension().tHeight-1;
				change = true;
			}
		}
		if (change) {
		//	glReadPixels(pos.tX, pos.tY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, m_Pixel);
			Point<int> inTexPos = { pos.tX - m_PrecisePalete->GetPosition().tX, pos.tY - m_PrecisePalete->GetPosition().tY };

			auto tex = m_PrecisePalete->GetFrontObject()->GetTexture();
			auto paleteDim = m_PrecisePalete->GetDimension();

			GLubyte* texData = new GLubyte[tex->GetDimension().tWidth*tex->GetDimension().tHeight*4];
			glBindTexture(GL_TEXTURE_2D, tex->GetTexId());
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);

			inTexPos.tY = ((float)inTexPos.tY / (float)paleteDim.tHeight)*tex->GetDimension().tHeight;
			inTexPos.tX = ((float)inTexPos.tX / (float)paleteDim.tWidth)*tex->GetDimension().tWidth;

			size_t row = inTexPos.tY * tex->GetDimension().tWidth*4;
			size_t col = inTexPos.tX * 4;
			//if (row + col + 3 >= tex->GetDimension().tWidth*tex->GetDimension().tHeight * 4) return true;
			GLubyte r = texData[row + col + 0];
			GLubyte g = texData[row + col + 1];
			GLubyte b = texData[row + col + 2];
			GLubyte a = texData[row + col + 3];

			Color tint = m_BasicSlider->GetColor();
			tint.r *= (float)r/255;
			tint.g *= (float)g/255;
			tint.b *= (float)b/255;


			Color trueRGBA(
				(1.0f - (float)a/255) * 1.0f + ((float)a / 255) * tint.r,
				(1.0f - (float)a/255) * 1.0f + ((float)a / 255) * tint.g,
				(1.0f - (float)a/255) * 1.0f + ((float)a / 255) * tint.b,
				1.0f
			);

			m_Preview->SetColor(trueRGBA);
			m_PreciseSlider->SetPosition(pos);
			delete[] texData;
		}
		
		return true;
	}

	bool BasicClickHandler(Event* ev)
	{
		Point<int> pos;
		bool change = false;
		if (ev->GetEventType() == EventType::MouseClick) {
			MouseClickEvent* mce = (MouseClickEvent*)ev;
			pos = mce->GetPosition();
			change = true;
		} else if (ev->GetEventType() == EventType::MouseMove) {
			MouseMoveEvent* mme = (MouseMoveEvent*)ev;
			if (m_BasicPalete->GetState() == 2 || (m_BasicPalete->GetState() == 1 && m_BasicPalete->GetDragInfo() == true)) {
				pos = mme->GetPosition();
				change = true;

				if (pos.tX < m_BasicPalete->GetPosition().tX) pos.tX = m_BasicPalete->GetPosition().tX;
				if (pos.tY < m_BasicPalete->GetPosition().tY) pos.tY = m_BasicPalete->GetPosition().tY;
				if (pos.tX > m_BasicPalete->GetPosition().tX + m_BasicPalete->GetDimension().tWidth) pos.tX = m_BasicPalete->GetPosition().tX + m_BasicPalete->GetDimension().tWidth - 1;
				if (pos.tY > m_BasicPalete->GetPosition().tY + m_BasicPalete->GetDimension().tHeight) pos.tY = m_BasicPalete->GetPosition().tY + m_BasicPalete->GetDimension().tHeight - 1;
			}
		}
		if (change) {
			glReadPixels(pos.tX, pos.tY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, m_Pixel);
			m_PrecisePalete->SetColor(Color{ (int)m_Pixel[0], (int)m_Pixel[1], (int)m_Pixel[2], 255 }, Color{ (int)m_Pixel[0], (int)m_Pixel[1], (int)m_Pixel[2], 255 }, Color{ (int)m_Pixel[0], (int)m_Pixel[1], (int)m_Pixel[2], 255 });
			m_BasicSlider->SetColor(Color{ (int)m_Pixel[0], (int)m_Pixel[1], (int)m_Pixel[2], 255 });
			m_BasicSlider->SetPosition({ pos.tX, m_BasicSlider->GetPosition().tY });
			auto p = m_PreciseSlider->GetPosition();
			glReadPixels(p.tX, p.tY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, m_Pixel);
			m_Preview->SetColor(Color{ (int)m_Pixel[0], (int)m_Pixel[1], (int)m_Pixel[2], 255 });
		}
		return true;
	}

	bool DragHandler(Event* ev)
	{
		if (ev->GetEventType() == EventType::MouseClick) {
			MouseClickEvent* mce = (MouseClickEvent*)ev;
			//std::cout << "ASDASDASD: " << mce->GetAction() << std::endl;
			if (mce->GetButton() == 0 && mce->GetAction() == 1){
				m_Drag = true;

				m_DragStart = MousePosition();//{p.x, p.y};
				
				m_StartPos = m_MainWindow->GetPosition();
			} else if (mce->GetButton() == 0 && mce->GetAction() == 0 && m_Drag == true){
				//std::cout << "RELEASE" << std::endl;
				m_Drag = false;
			}
		} else if (ev->GetEventType() == EventType::MouseMove) {
			MouseMoveEvent* mme = (MouseMoveEvent*)ev;

			if (m_Drag == true) {
				//POINT p;
				//GetCursorPos(&p);
				Point<int> newPos = MousePosition();
				//newPos.Set(p.x, p.y);
				auto diff = newPos-m_DragStart;
				//std::cout << diff.ToString() << std::endl;
				m_MainWindow->SetPosition(diff+ m_StartPos);
			}
		}
		return true;
	}


	void EntryPoint() override
	{
		//Main window
		m_MainWindow = new Window(nullptr, "Color Picker", { {400,300}, {645, 300}, WindowFlags::AutoInvokeRender | WindowFlags::FifoDrawable | WindowFlags::ChildAutoOperate | WindowFlags::Borderless });
		m_MainWindow->SetVSync(true);
		m_MainWindow->Init();
		auto dim = m_MainWindow->GetDimension();
		dim += {1, 1};

		//Load resources
		Texture paletteTex("Graphics/colorpalette.png");
		Texture bPaletteTex("Graphics/colorpalettebetter2.png");
		Texture circleTex("Graphics/circle.png");
		Texture circleBottomTex("Graphics/circlebottom.png");
		Font font("segoeui.ttf");
		font.LoadFontSize(18, 0, { 1024, 1024 });
		font.LoadFontSize(10);

		//Top panel
		Window header(m_MainWindow, "Header", { {0,  (int)dim.tHeight - 48 }, {dim.tWidth, 48} });
		header.Init();
		m_HeaderBg = new SimpleButton<Background>(
			&header,
			{ {0,0}, {0,0},  Anchor::Left | Anchor::Bottom | ResizePolicy::AutoFill },
			{ {{242, 245, 250, 255}, {230,233,235,255}}, {{242, 245, 250, 255}, {230,233,235,255}}, {{242, 245, 250, 255}, {230,233,235,255}}, {1,1} , { 18, {0,0,0,255},  &font}}, L""
		);
		m_HeaderBg->SetCallback(I_BIND(ColorPicker, DragHandler));
		m_HeaderBg->SetBypassMoveEvWhileDrag(true);

		//Content panel
		Window content(m_MainWindow, "Content", { {0, 0}, dim });
		content.Init();
		m_Bg = new Background(&content, { { 0,0 }, { 0,0 },  Anchor::Left | Anchor::Bottom | ResizePolicy::AutoFill }, { {255, 255, 255, 255}, {230,233,235,255}, {1,1} });

		//Create view
		Text headerText(&header, { { 20,8 }, { 400, (size_t)font.GetLineHeight(18) }, Anchor::Left | Anchor::Bottom }, {18, {107,122, 138,255},  &font }, L"Color picker");
		
		
		//m_Preview = new Background(&content, { { 1,1 }, { 150, dim.tHeight-48 },  Anchor::Left | Anchor::Bottom }, { {255, 0, 0, 255}, {230,233,235,255}, {1,1} });

		m_BasicSlider = new Image(
			&content,
			{ {250,29}, {0,0}, Anchor::VerticalCenter | Anchor::HorizontalCenter | ImageFlags::DetectSize },
			{{255,255,255,255}, {0,0,0,0}, {0,0} },
			&circleBottomTex
		);

		m_Quit = new SimpleButton<Background>(
			&header,
			{ {(int)dim.tWidth-100,6}, {80,34},  Anchor::Left | Anchor::Bottom },
			{ {{130,133,135,255}, {230,233,235,255}},  {{180,183,185,255}, {230,233,235,255}},  {{130,133,135,255}, {230,233,235,255}}, {1,1}, {18, {255,255,255,255}, &font} }, L"Quit"
		);


		m_Pick = new SimpleButton<Background>(
			&header,
			{ {(int)dim.tWidth - 200,6}, {80,34},  Anchor::Left | Anchor::Bottom },
			{ {{130,133,135,255}, {230,233,235,255}},  {{180,183,185,255}, {230,233,235,255}},  {{130,133,135,255}, {230,233,235,255}}, {1,1}, {18, {255,255,255,255}, &font} }, L"Pick"
		);


		m_BasicPalete = new SimpleButton<Image>(
			&content,
			{ {180,24}, {425,10},  Anchor::Left | Anchor::Bottom },
			{ {{255,255,255,255}, {0,0,0,0}}, {{255,255,255,255}, {0,0,0,0}}, {{255,255,255,255}, {0,0,0,0}}, {0,0}, {0, {0,0,0,0}, &font} }, L"", &paletteTex
		);
		m_BasicPalete->SetCallback(I_BIND(ColorPicker, BasicClickHandler));
		m_BasicPalete->SetBypassMoveEvWhileDrag(true);

		m_PrecisePalete = new SimpleButton<Image>(
			&content,
			{ {150,60}, {dim.tWidth - 152,dim.tHeight - 60-48}, Anchor::Left | Anchor::Bottom },
			{ {{255,255,255,255}, {0,0,0,0}}, {{255,255,255,255}, {0,0,0,0}}, {{255,255,255,255}, {0,0,0,0}}, {0,0}, {0, {0,0,0,0}, &font} }, L"", &bPaletteTex
		);
		m_PrecisePalete->SetCallback(I_BIND(ColorPicker, PreciseClickHandler));
		m_PrecisePalete->SetBypassMoveEvWhileDrag(true);

		m_PreciseSlider = new Image(
			&content,
			{ {-50,-50}, {0,0}, Anchor::VerticalCenter | Anchor::HorizontalCenter | ImageFlags::DetectSize },
			{ {255,255,255,255}, {0,0,0,0}, {0,0} },
			&circleTex
		);

		m_Preview = new ColorInfo(&content, { { 1,1 }, { 150, dim.tHeight - 48 },  Anchor::Left | Anchor::Bottom }, &font);

		while (!IfDone() && m_MainWindow->IfOpen() && m_MainWindow->Operate())
		{
			m_MainWindow->ClearScene();
			if (m_Quit->GetState() == 2 && m_Quit->GetDragInfo() == false) {
				m_Picked = false;
				m_MainWindow->Close();
			}

			if (m_Pick->GetState() == 2 && m_Pick->GetDragInfo() == false) {
				m_Picked = true;
				m_MainWindow->Close();
			}
		
		}
		Done();

		//Cleanup is here, because you can call window destructor only within thread in which window was created.
		delete m_Preview;
		delete m_Bg;
		delete m_HeaderBg;
		delete m_BasicPalete;
		delete m_PrecisePalete;
		delete m_Pick;
		delete m_Quit;
		delete m_BasicSlider;
		delete m_PreciseSlider;
		delete m_MainWindow;

	}

	//-----------------------------------------------------

	//Data variables
	Point<int> m_ClickPosition;
	unsigned char  m_Pixel[3];
	bool m_Picked;

	//Dragable variables
	Point<int> m_StartPos;
	Point<int> m_DragStart;
	bool m_Drag;

	//Scene variables
	Window* m_MainWindow;
	ColorInfo* m_Preview;
	Background* m_Bg;
	SimpleButton<Background>* m_HeaderBg;
	SimpleButton<Image>* m_BasicPalete;
	SimpleButton<Image>* m_PrecisePalete;
	SimpleButton<Background>* m_Pick;
	SimpleButton<Background>* m_Quit;
	Image* m_BasicSlider;
	Image* m_PreciseSlider;
};




int main()
{
	ColorPicker cp;
	if (cp.IfPicked()) {
		auto c = cp.GetColor();
		std::cout << c.r << "    " << c.g << "   " << c.b << std::endl;
	}

	return 0;
}
