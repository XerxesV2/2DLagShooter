#include "TextInputBox.hpp"
#include "SharedVariables.hpp"

TextInputBox::TextInputBox(sf::Font& font, sf::RenderWindow& window, std::deque<uint32_t>& keyCodes)
	: m_Font(font), m_Window(window), m_PressedKeyCodes(keyCodes)
{
	m_AutoText.setFont(m_Font);
	m_AutoText.setCharacterSize(50);
	//m_AutoText.setOutlineColor(sf::Color::Black);
	m_AutoText.setFillColor(sf::Color(100, 100, 100, 200));
	m_AutoText.setOutlineThickness(3.f);
	m_AutoText.setString("");

	m_Text.setFont(m_Font);
	m_Text.setCharacterSize(50);
	m_Text.setOutlineColor(sf::Color::Black);
	m_Text.setOutlineThickness(3.f);
	m_Text.setString("ENTER NAME HERE");
	m_Str = "";

	m_LowerUcBound = 48;
	m_UpperUcBound = 126;

	m_Box.setSize({ 400.f, 100.f });
	m_Box.setOrigin( 400.f / 2.f, 100.f / 2.f);
	m_Box.setFillColor(sf::Color(50, 50, 50, 255));
	m_Box.setOutlineThickness(5.f);

	m_Text.setOrigin(0.f, m_Text.getGlobalBounds().height / 2.f + m_Text.getLocalBounds().top);
	m_Text.setPosition(m_Box.getGlobalBounds().left + m_Box.getOutlineThickness(), m_Box.getGlobalBounds().top + m_Box.getGlobalBounds().height / 2.f);
}

TextInputBox::~TextInputBox()
{
}

bool TextInputBox::Update()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right))
		if (m_Box.getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(m_Window)) / ((float)m_Window.getSize().y / g_CamSizeY))) {
			m_PressedKeyCodes.clear();
			m_bBlinkState = true;
			ForceUpdateTextCursor();
			if (m_bFirstActive) {
				m_Str.clear();
				m_Text.setString("");
				m_bFirstActive = false;
			}
			m_bActive = true;
		}
		else {
			m_bActive = false;
			m_bBlinkState = false;
			ForceUpdateTextCursor();
		}

	if (!m_bActive) return false;

	//m_Text.setPosition(sf::Vector2f(sf::Mouse::getPosition(m_Window)));

	UpdateTextCursor();
	return HandleKeyboardInput();
}

void TextInputBox::SetPos(sf::Vector2f& pos)
{
	m_Box.setPosition(pos);
	m_Text.setPosition(m_Box.getGlobalBounds().left + m_Box.getOutlineThickness(), m_Box.getGlobalBounds().top + m_Box.getGlobalBounds().height / 2.f);
}

void TextInputBox::SetPos(sf::Vector2f pos)
{
	m_Box.setPosition(pos);
	m_Text.setPosition(m_Box.getGlobalBounds().left + m_Box.getOutlineThickness(), m_Box.getGlobalBounds().top + m_Box.getGlobalBounds().height / 2.f);
}

void TextInputBox::SetCharacterSize(uint32_t size)
{
	m_Text.setCharacterSize(size);
	m_AutoText.setCharacterSize(size);
	m_Text.setOrigin(0.f, m_Text.getGlobalBounds().height / 2.f + m_Text.getLocalBounds().top);
	m_Text.setPosition(m_Box.getGlobalBounds().left + m_Box.getOutlineThickness(), m_Box.getGlobalBounds().top + m_Box.getGlobalBounds().height / 2.f);
}

void TextInputBox::SetAlpha(uint8_t alpha)
{
	sf::Color col = m_Box.getFillColor();
	col.a = alpha;
	m_Box.setFillColor(col);
	col = m_Box.getOutlineColor();
	col.a = alpha;
	m_Box.setOutlineColor(col);
	col = m_Text.getFillColor();
	col.a = alpha;
	m_Text.setFillColor(col);
}

void TextInputBox::SetUnicodeBounds(uint32_t lower, uint32_t upper)
{
	m_LowerUcBound = lower;
	m_UpperUcBound = upper;
}

void TextInputBox::draw(sf::RenderTarget& target, const sf::RenderStates states) const
{
	target.draw(m_Box);
	target.draw(m_Text);
	target.draw(m_AutoText);
}

void TextInputBox::UpdateTextCursor()
{
	if (m_CursorBlinkClock.getElapsedTime().asSeconds() > m_BlinkTime) {

		m_BlinkTime = m_CursorBlinkClock.getElapsedTime().asSeconds() + m_BlinkSpeed;
		m_bBlinkState = !m_bBlinkState;
		m_Text.setString(m_Str + (m_bBlinkState ? '_' : ' '));
	}
}

void TextInputBox::UpdateAutoComplete()
{
	if (m_pAutoCompleteCommands == nullptr) return;
	if (m_Str.empty() || m_Str.front() != '/') return;

	std::string strToSearch = m_Str.substr(m_Str.find_last_of(' ')+1);
	if (!strToSearch.empty() && strToSearch.front() == '/')
		strToSearch.erase(0, 1);

	const std::string* pBestStr = nullptr;
	int i = 0;
	int bestCount = 0;

	auto sfunc = [&](const std::string& word)
	{
		int count = 0;
		int j = 0;
		for (auto& ch : word)
		{
			if (strToSearch.size() <= j) break;
			if (ch == strToSearch.at(j))
				++count;
			else
				break;
			++j;
		}
		if (count > bestCount) {
			bestCount = count;
			pBestStr = &word;
		}
		++i;
	};

	for (auto& word : *m_pAutoCompleteCommands)
	{
		sfunc(word);
	}


	i = 0;
	for (auto& [word, id] : *m_pNameList)
	{
		sfunc(word);
	}
	

	if (pBestStr != nullptr) {
		m_AutoText.setString(*pBestStr);
		m_AutoText.setPosition(m_Text.getGlobalBounds().left + m_Text.getLocalBounds().width, m_Text.getGlobalBounds().top - m_Text.getLocalBounds().top);
	}
	else {
		m_AutoText.setString("");
		m_AutoText.setPosition(m_Text.getGlobalBounds().left + m_Text.getLocalBounds().width, m_Text.getGlobalBounds().top - m_Text.getLocalBounds().top);
	}
}

void TextInputBox::ForceUpdateTextCursor()
{
	m_Text.setString(m_Str + (m_bBlinkState ? '_' : ' '));
}

bool TextInputBox::HandleKeyboardInput()
{
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
		m_PressedKeyCodes.clear();
		m_bFirstActive = true;
		if (m_Histroy.size() >= 10)
			m_Histroy.pop_back();
		m_Histroy.push_front(m_Str);
		m_HisytoryIndex = -1;
		m_Text.setString("");
		m_AutoText.setString("");
		return true;
	}
	if (m_PressedKeyCodes.empty()) return false;

	for (auto& key : m_PressedKeyCodes) {
		if (key > m_LowerUcBound && key < m_UpperUcBound) {
			if (m_Str.size() >= m_MaxinputLength) continue;
			m_Str += (char)key;
			continue;
		}
		switch (key)
		{
		case 127: //LCtrl BACKSPACE
		{
			if (m_Str.empty()) continue;
			size_t pos = m_Str.find_last_of(' ');
			if (pos != std::string::npos)
				m_Str.erase(pos, m_Str.size() - 1);
			else
				m_Str.erase(0, m_Str.size());
		}break;
		case 8:	//BACKSPACE
		{
			if (m_Str.empty()) continue;
			m_Str.erase(m_Str.size() - 1);
		}break;
		case 9:	//TAB
		{
			size_t pos = m_Str.find_last_of(' ');
			if (pos == std::string::npos)
				m_Str.replace(1, m_AutoText.getString().getSize(), m_AutoText.getString());
			else
				m_Str.replace(pos + 1, m_AutoText.getString().getSize(), m_AutoText.getString());
		}break;
		case 1001:	//UP ARROW
		{
			if (m_HisytoryIndex < (int)m_Histroy.size()-1)
				m_Str = m_Histroy.at(++m_HisytoryIndex);

		} break;
		case 1002:	//DOWN ARROW
		{
			if (m_HisytoryIndex > 0u)
				m_Str = m_Histroy.at(--m_HisytoryIndex);
		} break;
		default: break;
		}

	}
	m_PressedKeyCodes.clear();
	m_Text.setString(m_Str + (m_bBlinkState ? '_' : ' '));
	UpdateAutoComplete();
	return false;
}
