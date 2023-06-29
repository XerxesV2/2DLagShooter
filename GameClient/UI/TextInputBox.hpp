#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <string_view>
#include <deque>
#include <unordered_map>

class TextInputBox : public sf::Drawable
{
public:
	TextInputBox(sf::Font& font, sf::RenderWindow& window, std::deque<uint32_t>& keyCodes);
	~TextInputBox();

	// true means ENTER was pressed
	bool Update();

	const std::string_view const GetTextView() { return m_Str; }
	std::string& GetText() { return m_Str; }
	//void SetPos(sf::Vector2f&& pos) { m_Box.setPosition(pos); }
	void SetPos(sf::Vector2f& pos);
	void SetPos(sf::Vector2f pos);
	void SetSize(sf::Vector2f&& size) { m_Box.setSize(size); m_Box.setOrigin(size.x / 2.f, size.y / 2.f); }
	void SetSize(sf::Vector2f& size) { m_Box.setSize(size); m_Box.setOrigin(size.x / 2.f, size.y / 2.f); }
	//void SetSize(sf::Vector2f size) { m_Box.setSize(size); m_Box.setOrigin(size.x / 2.f, size.y / 2.f); }

	const sf::Vector2f& GetPos() const { return m_Box.getPosition(); }
	const sf::Vector2f& GetSize() const { return m_Box.getSize(); }
	const sf::FloatRect& GetGlobalBounds() const { return m_Box.getGlobalBounds(); }
	sf::Color& GetFillColor() { m_Box.getFillColor(); }
	sf::Color& GetOutlineColor(const sf::Color& color) { m_Box.getOutlineColor(); }
	const bool IsActive() const { return m_bActive; }

	void SetActive(bool active) { m_bActive = active; }

	void SetMaxInputLegth(uint32_t length) { m_MaxinputLength = length; }
	void SetHintText(std::string_view text) { m_Text.setString(text.data()); }
	void SetFillColor(const sf::Color& color) { m_Box.setFillColor(color); }
	void SetOutlineColor(const sf::Color& color) { m_Box.setOutlineColor(color); }
	void SetTextOutlineColor(const sf::Color& color) { m_Text.setOutlineColor(color); }
	void SetOutlineThickness(const float thic) { m_Box.setOutlineThickness(thic); }
	void SetTextOutlineThickness(const float thic) { m_Text.setOutlineThickness(thic); }
	void SetCharacterSize(uint32_t size);
	void SetAlpha(uint8_t alpha);
	void SetAutoComplete(std::vector<std::string>* commands) { m_pAutoCompleteCommands = commands; };
	void SetNameList(std::unordered_map<std::string, uint32_t>* names) { m_pNameList = names; };

	void SetUnicodeBounds(uint32_t lower, uint32_t upper);

	void Clear() { m_Str.clear(); m_Text.setString(""); }

	void draw(sf::RenderTarget& target, const sf::RenderStates states = sf::RenderStates::Default) const override;

private:
	void UpdateTextCursor();
	void UpdateAutoComplete();
	void ForceUpdateTextCursor();
	bool HandleKeyboardInput();
private:
	sf::Font& m_Font;
	sf::RenderWindow& m_Window;
	std::deque<uint32_t>& m_PressedKeyCodes;
	std::vector<std::string>* m_pAutoCompleteCommands = nullptr;
	std::deque<std::string> m_Histroy;
	std::unordered_map<std::string, uint32_t>* m_pNameList = nullptr;
	sf::Text m_AutoText;

	sf::Text m_Text;
	std::string m_Str;
	sf::RectangleShape m_Box;
	sf::Clock m_CursorBlinkClock;
	uint32_t m_MaxinputLength;
	uint32_t m_LowerUcBound = 0u;
	uint32_t m_UpperUcBound = 0u;
	int m_HisytoryIndex = -1;
	float m_BlinkTime = 0.f;
	float m_BlinkSpeed = 0.5f;

	bool m_bActive = false;
	bool m_bFirstActive = true;
	bool m_bBlinkState = false;
};

