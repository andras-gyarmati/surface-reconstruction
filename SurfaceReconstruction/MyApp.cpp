#include "MyApp.h"

#include <math.h>
#include <vector>

#include <array>
#include <list>
#include <tuple>

#include <random>

#include "imgui\imgui.h"

#include "Includes/ObjParser_OGL3.h"

CMyApp::CMyApp(void)
{
    // m_camera.SetView(glm::vec3(5, 5, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}


CMyApp::~CMyApp(void)
{
    
}

std::vector<Vertex> loadPLYFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    std::string line;
    int numVertices = 0;
    bool headerFinished = false;
    while (std::getline(file, line)) {
        if (line == "end_header") {
            headerFinished = true;
            break;
        }

        if (line.find("element vertex") == 0) {
            std::sscanf(line.c_str(), "element vertex %d", &numVertices);
        }
    }

    if (!headerFinished) {
        std::cerr << "Error: Could not find end_header in PLY file" << std::endl;
        return {};
    }

    std::vector<Vertex> vertices(numVertices);
    for (int i = 0; i < numVertices; ++i) {
        file >> vertices[i].x >> vertices[i].y >> vertices[i].z;
        file >> vertices[i].r >> vertices[i].g >> vertices[i].b;
    }

    return vertices;
}

//
// egy parametrikus felület (u,v) paraméterértékekhez tartozó pontjának
// kiszámítását végző függvény
//
glm::vec3 CMyApp::GetSpherePos(float u, float v)
{
    // origó középpontú, egységsugarú gömb parametrikus alakja: http://hu.wikipedia.org/wiki/G%C3%B6mb#Egyenletek 
    // figyeljünk:	matematikában sokszor a Z tengely mutat felfelé, de nálunk az Y, tehát a legtöbb képlethez képest nálunk
    //				az Y és Z koordináták felcserélve szerepelnek
    u *= 2 * 3.1415f;
    v *= 3.1415f;
    float r = 2;

    return glm::vec3(r * sin(v) * cos(u), r * cos(v), r * sin(v) * sin(u)) 
        + glm::vec3(1.0f, 0.f, 0.f); // plusz eltolas hogy benne legyunk a boidok kozott es latszodjon az eger mozgas
}

bool CMyApp::Init()
{
    // vetítési mátrix létrehozása
    m_matProj = glm::perspective(45.0f, 640 / 480.0f, 1.0f, 1000.0f);

    // törlési szín legyen fehér
    glClearColor(0.1f,0.1f,0.41f, 1);

    // most nincs hátlapeldobás, szeretnénk látni a hátlapokat is, de csak vonalakra raszterizálva
    //glEnable(GL_CULL_FACE); // kapcsoljuk be a hatrafele nezo lapok eldobasat

    glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)

    glLineWidth(4.0f); // a vonalprimitívek vastagsága: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glLineWidth.xhtml
    glPointSize(15.0f); // a raszterizált pontprimitívek mérete

    // átlátszóság engedélyezése
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // meghatározza, hogy az átlátszó objektum az adott pixelben hogyan módosítsa a korábbi fragmentekből oda lerakott színt: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml

    //
    // shaderek betöltése
    //

    // tengelyeket kirajzoló shader rövid inicializálása
    m_axesProgram.Init({
                {GL_VERTEX_SHADER,		"axes.vert"},
                {GL_FRAGMENT_SHADER,	"axes.frag"}
    });

    // kockát kirajzoló program
    m_passthroughProgram.Init({
        { GL_VERTEX_SHADER,		"passthrough.vert" },
        { GL_FRAGMENT_SHADER,	"passthrough.frag" }
    },
    {
        {0, "vs_in_pos"}
    });

    // részecskéket kirajzoló program
    m_particleProgram.Init({	// shaderek felsorolása
        { GL_VERTEX_SHADER,		"particle.vert" },
        { GL_FRAGMENT_SHADER,	"particle.frag" }
    },
    {	// binding-ok felsorolása
        { 0, "vs_in_pos" },
        { 1, "vs_in_vel" },
    });

    m_program.Init({
        { GL_VERTEX_SHADER, "myVert.vert" },
        { GL_FRAGMENT_SHADER, "myFrag.frag" }
    },
    {
        { 0, "vs_in_pos" },					// VAO 0-as csatorna menjen a vs_in_pos-ba
        { 1, "vs_in_normal" },				// VAO 1-es csatorna menjen a vs_in_normal-ba
    });

    //
    // geometria definiálása (std::vector<...>) és GPU pufferekbe (m_buffer*) való feltöltése BufferData-val
    //

    // vertexek pozíciói:
    /*
    Az m_gpuBufferPos konstruktora már létrehozott egy GPU puffer azonosítót és a most következő BufferData hívás ezt 
        1. bind-olni fogja GL_ARRAY_BUFFER target-re (hiszen m_gpuBufferPos típusa ArrayBuffer) és
        2. glBufferData segítségével áttölti a GPU-ra az argumentumban adott tároló értékeit

    */
    m_gpuBufferPos.BufferData( 
        std::vector<glm::vec3>{
            // hátsó lap
            glm::vec3(-1,-1,-1),
            glm::vec3( 1,-1,-1), 
            glm::vec3( 1, 1,-1),
            glm::vec3(-1, 1,-1),
            // elülső lap
            glm::vec3(-1,-1, 1),
            glm::vec3( 1,-1, 1),
            glm::vec3( 1, 1, 1),
            glm::vec3(-1, 1, 1),

        }
    );

    // és a primitíveket alkotó csúcspontok indexei (az előző tömbökből) - triangle list-el való kirajzolásra felkészülve
    m_gpuBufferIndices.BufferData(
        std::vector<int>{
            // hátsó lap
            0, 1, 2,
            2, 3, 0,
            // elülső lap
            4, 6, 5,
            6, 4, 7,
            // bal
            0, 3, 4,
            4, 3, 7,
            // jobb
            1, 5, 2,
            5, 6, 2,
            // alsó
            1, 0, 4,
            1, 4, 5,
            // felső
            3, 2, 6,
            3, 6, 7,
    }
    );

    // geometria VAO-ban való regisztrálása
    m_vao.Init(
        {
            { CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpuBufferPos },		// 0-ás attribútum "lényegében" glm::vec3-ak sorozata és az adatok az m_gpuBufferPos GPU pufferben vannak
        },
        m_gpuBufferIndices
    );
    
    // kamera
    // m_camera.SetProj(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 1000.0f);

    // részecskék inicializálása
    m_particlePos.reserve(m_particleCount);
    m_particleVel.reserve(m_particleCount);

    // véletlenszám generátor inicializálása
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rnd(-1,1);

    // CPU oldali részecsketömbök feltöltése
    for (int i = 0; i < m_particleCount; ++i)
    {
        m_particlePos.push_back( glm::vec3(rnd(gen), rnd(gen), rnd(gen)) );
        m_particleVel.push_back( glm::vec3( 2*rnd(gen), 2*rnd(gen), 2*rnd(gen) ) );
    }

    // GPU-ra áttölteni a részecskék pozícióit
    m_gpuParticleBuffer.BufferData(m_particlePos);	// <=>	m_gpuParticleBuffer = m_particlePos;

    // és végül a VAO-t inicializálni
    m_gpuParticleVAO.Init({
        {CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpuParticleBuffer}
    });

    return true;
}

void CMyApp::Clean()
{
}

void CMyApp::Reset()
{
    // véletlenszám generátor inicializálása
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rnd(-1, 1);
    m_particlePos.clear();
    m_particleVel.clear();
    for (int i = 0; i < m_particleCount; ++i)
    {
        m_particlePos.push_back(glm::vec3(rnd(gen), rnd(gen), rnd(gen)));
        m_particleVel.push_back(glm::vec3(2 * rnd(gen), 2 * rnd(gen), 2 * rnd(gen)));
    }
}

void CMyApp::Update()
{
    // nézeti transzformáció beállítása
    m_matView = glm::lookAt(m_eye, m_at, m_up);

    static Uint32 last_time = SDL_GetTicks();
    float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

    // m_camera.Update(delta_time);

    // calc velocity changes
    std::vector<glm::vec3> vels;
    for (int i = 0; i < m_particleCount; ++i)
    {
        vels.push_back(glm::vec3(0, 0, 0));
        
        // find neighbors
        std::vector<glm::vec3> nsp;
        std::vector<glm::vec3> nsv;
        for (int j = 0; j < m_particleCount; ++j)
        {
            if (i != j && glm::distance(m_particlePos[i], m_particlePos[j]) <= n_dist) {
                nsp.push_back(m_particlePos[j]);
                nsv.push_back(m_particleVel[j]);
            }
        }

        // coherence
        if (nsp.size() > 0) {
            glm::vec3 cog = glm::vec3(0, 0, 0);
            for (int j = 0; j < nsp.size(); ++j)
            {
                cog += nsp[j];
            }
            cog /= (float)nsp.size();
            glm::vec3 coherence = (cog - m_particlePos[i]) * a;
            vels[i] += coherence;
        }

        // separation
        for (int j = 0; j < nsp.size(); ++j)
        {
            vels[i] += (m_particlePos[i] - nsp[j]) * b;
        }

        // alignment
        if (nsv.size() > 0) {
            glm::vec3 n_avg_vel = glm::vec3(0, 0, 0);
            for (int j = 0; j < nsv.size(); ++j)
            {
                n_avg_vel += nsv[j];
            }
            n_avg_vel /= (float)nsv.size();
            glm::vec3 alignment = n_avg_vel * c;
            vels[i] += alignment;
        }
    }

    // frissítsük a pozíciókat
    static const float energyRemaining = 1;	// tökéletesen rugalmas ütközés
    for (int i = 0; i < m_particleCount; ++i)
    {
        // apply coherence, separation and alignment
        for (int j = 0; j < m_particleCount; ++j)
        {
            m_particleVel[i] += vels[i];
        }

        // limit velocity
        if (glm::length(m_particleVel[i]) > max_speed) {
            m_particleVel[i] = glm::normalize(m_particleVel[i]) * max_speed;
        }

        // change pos based on vel
        m_particlePos[i] += m_particleVel[i] * delta_time;
        
        /*if ((m_particlePos[i].x >= 1 && m_particleVel[i].x > 0) || (m_particlePos[i].x <= -1 && m_particleVel[i].x < 0))
            m_particleVel[i].x *= -energyRemaining;
        if ( (m_particlePos[i].y >= 1 && m_particleVel[i].y > 0) || (m_particlePos[i].y <= -1 && m_particleVel[i].y < 0))
            m_particleVel[i].y *= -energyRemaining;
        if ( (m_particlePos[i].z >= 1 && m_particleVel[i].z > 0) || (m_particlePos[i].z <= -1 && m_particleVel[i].z < 0))
            m_particleVel[i].z *= -energyRemaining;*/

        if ((m_particlePos[i].x >= 1 && m_particleVel[i].x > 0))
        {
            m_particlePos[i].x = 1;
            m_particleVel[i].x *= -energyRemaining;
        }
        if ((m_particlePos[i].x <= -1 && m_particleVel[i].x < 0))
        {
            m_particlePos[i].x = -1;
            m_particleVel[i].x *= -energyRemaining;
        }
        if ((m_particlePos[i].y >= 1 && m_particleVel[i].y > 0))
        {
            m_particlePos[i].y = 1;
            m_particleVel[i].y *= -energyRemaining;
        }
        if ((m_particlePos[i].y <= -1 && m_particleVel[i].y < 0))
        {
            m_particlePos[i].y = -1;
            m_particleVel[i].y *= -energyRemaining;
        }
        if ((m_particlePos[i].z >= 1 && m_particleVel[i].z > 0))
        {
            m_particlePos[i].z = 1;
            m_particleVel[i].z *= -energyRemaining;
        }
        if ((m_particlePos[i].z <= -1 && m_particleVel[i].z < 0))
        {
            m_particlePos[i].z = -1;
            m_particleVel[i].z *= -energyRemaining;
        }
    }

    // frissítsük a puffert
    glBindBuffer(GL_ARRAY_BUFFER, m_gpuParticleBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3)*m_particlePos.size(), &(m_particlePos[0][0]));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // használhattuk volna simán a 
    //m_gpuParticleBuffer = m_particlePos; // <=> m_gpuParticleBuffer.BufferData(m_particlePos);
    // sort is, viszont az egy rejtett glBufferData, ami pedig tanultuk, hogy két dolgot csinál(hat):
    //		1. MINDIG allokál a GPU-n
    //		2. HA nem nullptr-t adunk forrásnak, AKKOR a RAM-ból másol a GPU-ra is
    // Ez nyilván első látásra nem hatékony, hiszen a glBufferSubData nem allokál, csak másol, viszont
    // BSc grafikás dolgokon túlmutató ismeretekkel felvértezve már belátható, hogy annyira nem vészes, sőt...
    // - de ilyenekről szól a Haladó Grafika MSc :)

    // jegyezzuk meg az utolso update-et
    last_time = SDL_GetTicks();
}

void CMyApp::Render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 mvp = m_matProj * m_matView * m_matWorld;

    // tengelyek
    m_axesProgram.Use();
    m_axesProgram.SetUniform("mvp", mvp);

    glDrawArrays(GL_LINES, 0, 6);

    // kocka
    /*glPolygonMode(GL_BACK, GL_LINE);
    m_vao.Bind();

    m_passthroughProgram.Use();
    m_passthroughProgram.SetUniform("mvp", m_camera.GetViewProj());

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);*/

    // részecskék
    glEnable(GL_PROGRAM_POINT_SIZE);
    m_gpuParticleVAO.Bind();
    m_particleProgram.Use();
    m_particleProgram.SetUniform("mvp", mvp);

    glDrawArrays(GL_POINTS, 0, m_particleCount);

    glDisable(GL_PROGRAM_POINT_SIZE);

    // fal
    m_program.Use();

    // most kikapcsoljuk a hátlapeldobást, hogy lássuk mindkétt oldalát!
    // Feladat: még mi kellene ahhoz, hogy a megvilágításból származó színek jók legyenek?
    glDisable(GL_CULL_FACE);

    m_matWorld = glm::translate(glm::vec3(0, 0, 0));
    m_program.SetUniform("world", m_matWorld);
    m_program.SetUniform("worldIT", glm::transpose(glm::inverse(m_matWorld)));
    mvp = m_matProj * m_matView * m_matWorld;
    m_program.SetUniform("MVP", mvp);
    m_program.SetUniform("Kd", m_wallColor);
    

    m_vao.Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.Unbind(); // nem feltétlen szükséges: a m_mesh->draw beállítja a saját VAO-ját

    // de kapcsoljuk most vissza, mert még jön egy Suzanne
    glEnable(GL_CULL_FACE);

    m_program.Unuse();

    if (ImGui::Begin("Boids"))
    {
        ImGui::Text("Change the simulation properties");
        ImGui::SliderFloat("Coherence", &a, 0, 1.0);
        ImGui::SliderFloat("Separation", &b, 0, 1.0);
        ImGui::SliderFloat("Alignment", &c, 0, 1.0);
        ImGui::SliderFloat("Max speed", &max_speed, 0, 1.0);
        ImGui::SliderFloat("Neighbor distance", &n_dist, 0, 1.0);
        if (ImGui::Button("Reset particle pos")) {
            CMyApp::Reset();
        }
        ImGui::SliderFloat4("Fal kd", &(m_wallColor[0]), 0, 1);
    }
    ImGui::End();

    //
    // UI
    //
    // A következő parancs megnyit egy ImGui tesztablakot és így látszik mit tud az ImGui.
    // ImGui::ShowTestWindow();
    // A ImGui::ShowTestWindow implementációja az imgui_demo.cpp-ben található
    // Érdemes még az imgui.h-t böngészni, valamint az imgui.cpp elején a FAQ-ot elolvasni.
    // Rendes dokumentáció nincs, de a fentiek elegendőek kell legyenek.

    /*
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_FirstUseEver);
    // csak akkor lépjünk be, hogy ha az ablak nincs csíkká lekicsinyítve...
    if (ImGui::Begin("Physics control"))	
    {
        

    }
    ImGui::End(); // ...de még ha le is volt, End()-et hívnunk kell
    */

    /*

    0. feladat: ne legyen tökéletesen rugalmas az ütközés, hanem UI-ról állítható legyen a mozgási energia megmaradt aránya!
    1. feladat: hasson a részecskékre gravitáció! Tedd fel, hogy minden részecske egységnyi tömegű. 
    2. feladat: a részecskék ütközzenek egymással is!
    3. feladat: rajzold ki minden egyes részecskéhez egy GL_LINES-zal a sebességvektorát!
    4. feladat: az UI segítségével lehessen külön-külön
        a) újragenerálni a véletlen pozíciókat
        b) újragenerálni a véletlen sebességeket
        c) növelni és csökkenteni a részecskék számát
    */
}

glm::vec3 CMyApp::ToDescartes(float fi, float theta)
{
    return glm::vec3(
        sinf(theta) * cosf(fi),
        cosf(theta),
        sinf(theta) * sinf(fi));
}

void CMyApp::KeyboardDown(SDL_KeyboardEvent& key)
{
    // m_camera.KeyboardDown(key);
    switch (key.keysym.sym)
    {
        case(SDLK_w): 
            std::cout << "---\n|W|\n"; 
            m_v -= step_size;
            m_v = glm::mod(m_v, 2.0f);
            m_at = CMyApp::GetSpherePos(m_u, m_v);
            m_eye = m_at - m_fw;
            break;
        case(SDLK_s): 
            std::cout << "---\n|S|\n"; 
            std::cout << "---\n|W|\n";
            m_v += step_size;
            m_v = glm::mod(m_v, 2.0f);
            m_at = CMyApp::GetSpherePos(m_u, m_v);
            m_eye = m_at - m_fw;
            break;			break;
        case(SDLK_d): 
            std::cout << "---\n|D|\n";
            std::cout << "---\n|W|\n";
            m_u += step_size;
            m_u = glm::mod(m_u, 1.0f);
            m_at = CMyApp::GetSpherePos(m_u, m_v);
            m_eye = m_at - m_fw;
            break;			break;
        case(SDLK_a): 
            std::cout << "---\n|A|\n"; 
            std::cout << "---\n|W|\n";
            m_u -= step_size;
            m_u = glm::mod(m_u, 1.0f);
            m_at = CMyApp::GetSpherePos(m_u, m_v);
            m_eye = m_at - m_fw;
            break;			break;
        default: 
            break;
    }
}

void CMyApp::KeyboardUp(SDL_KeyboardEvent& key)
{
    // m_camera.KeyboardUp(key);
}

void CMyApp::MouseMove(SDL_MouseMotionEvent& mouse)
{
    // m_camera.MouseMove(mouse);
    if (m_is_left_pressed)
    {
        m_fi += mouse.xrel / 100.0f;
        m_theta += mouse.yrel / 100.0f;
        m_theta = glm::clamp(m_theta, 0.1f, 3.1f);
        m_fw = ToDescartes(m_fi, m_theta);
        m_eye = m_at - m_fw;
        m_left = glm::cross(m_up, m_fw);
    }
}

void CMyApp::MouseDown(SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = true;
}

void CMyApp::MouseUp(SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = false;
}

void CMyApp::MouseWheel(SDL_MouseWheelEvent& wheel)
{
}

// a két paraméterben az új ablakméret szélessége (_w) és magassága (_h) található
void CMyApp::Resize(int _w, int _h)
{
    glViewport(0, 0, _w, _h );

    // m_camera.Resize(_w, _h);
    m_matProj = glm::perspective(glm::radians(60.0f),	// 60 fokos nyílásszög radiánban
        _w / (float)_h,			// ablakméreteknek megfelelő nézeti arány
        0.01f,					// közeli vágósík
        1000.0f);				// távoli vágósík
}