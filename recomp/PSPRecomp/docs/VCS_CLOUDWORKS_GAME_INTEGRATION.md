# CloudWorks: integração 1:1 com o estado do GTA VCS

## Estado atual

O shader usa os três perfis físicos, march adaptativo, sombras, extinção,
atmosfera e composição front-to-back do CloudWorks/ProperShaders. A câmera vem
do estado autoritativo do jogo (`TheCamera + 0x9B0`) e o passe é inserido no
world target depois dos opacos e antes do primeiro grupo equivalente a
`FadingEntities`.

Os valores que o ProperShaders de San Andreas obtém do timecycle e de
`CWeather` ainda são placeholders em `ProperShaders.ini`. Isso é intencional:
ligar um endereço não confirmado ao shader produz mudanças de clima erradas ou
leituras inválidas e não constitui uma integração 1:1.

## Fontes que precisam ser identificadas no guest

Para substituir cada placeholder, localizar no ELF do VCS e validar em runtime:

| Campo do INI | Fonte conceitual do ProperShaders/SA | Evidência necessária no VCS |
|---|---|---|
| `CoverageLow/Mid/High` | grupos de clima atual/anterior e interpolação | observar transições sunny/cloudy/rain/fog e reconstruir os três pesos |
| `SunDirection*` | direção real do sol ou lua | vetor unitário acompanhando hora; confirmar eixos do mundo do VCS |
| `DayProgression` | componente vertical da direção solar | faixa e pontos de troca sol/lua confirmados durante um ciclo completo |
| `SunColor*` | cor da corona/timecycle, modulada pelo clima | identificar RGB linear versus 0–255 e interpolação old/new weather |
| `CloudBaseColor*` | cor da base das nuvens do timecycle | separar da cor do céu/horizonte e validar dia, pôr do sol, noite e chuva |
| `AtmosphereDensity` | intensidade de chuva | localizar intensidade interpolada, não apenas o ID do clima |
| `Mist` | chuva, neblina e cloudiness combinadas | validar as três intensidades continuamente interpoladas |
| `FogColor*` | cor inferior do céu/horizonte | confirmar espaço de cor e ordem RGB |
| `FogStart` | plano inicial de fog da câmera | comparar com comandos GE `FOG1/FOG2` e o far clip do frame |
| `Speed` | tempo em segundos e vento | relógio monotônico do jogo + intensidade de vento interpolada |
| `RandomSeed` | seed de início/save, limitado e convertido em fase | escolher estatística estável e aplicar wrap em `2*pi` |

O relógio já tem uma pista confirmada: `gp + 0x1DE0` (hora) e
`gp + 0x1DE1` (minuto), usados por Project2DFX. Isso ajuda a localizar as
rotinas adjacentes de `CClock`, mas não substitui a direção solar contínua.

## Método de investigação

1. Encontrar referências às variáveis de hora conhecidas no AOT/disassembly e
   seguir os consumidores que atualizam iluminação, céu e clima.
2. Instrumentar somente candidatos concretos, no máximo uma vez por frame, em
   um log contendo hora, weather IDs/interpolação, vetores e cores.
3. Capturar uma rota reproduzível cobrindo meio-dia, pôr do sol, noite,
   amanhecer, chuva e neblina. Confirmar continuidade e faixa de cada campo.
4. Criar um `VcsCloudWeatherState` lido do `GuestMemory` no mesmo ponto em que a
   câmera autoritativa é capturada. Nunca ler guest memory no pixel/draw hot path.
5. Manter fallback por campo: valor guest validado quando finito e dentro da
   faixa; caso contrário, placeholder do INI e aviso limitado no log.
6. Comparar screenshots pareadas contra o ProperShaders com as mesmas entradas
   numéricas antes de ajustar qualquer constante artística.

## Renderização e ordem

O PSP não expõe um marcador chamado `RenderFadingEntities`. A fronteira atual é
inferida pelo estado GE: após geometria do world target com depth test+write,
sem blend/alpha test, o primeiro draw com blend, alpha test ou depth sem write é
tratado como o início dos objetos transparentes. O passe de nuvens é gravado
imediatamente antes desse draw, com depth `EQUAL` ao clear reverso (zero), para
preencher somente céu e permitir que árvores/grades/partículas sejam desenhadas
por cima.

Essa heurística deve ser validada em interiores, água, chuva, reflexos e efeitos
de missão. Se houver falsos limites, o passo seguinte é identificar o PC guest
da função que emite o primeiro draw de fading e transportar um marcador de fase
para o backend GE.

## Critérios para considerar a integração concluída

- Nenhum parâmetro visual de clima permanece obrigatório no INI.
- Transições old/new weather são contínuas e não saltam entre frames.
- Sol, lua, fog e base das nuvens acompanham o timecycle em um ciclo de 24 h.
- Nuvens ficam fixas no mundo durante rotação e translação da câmera.
- Opacos ocluem as nuvens; fading/folhagem/partículas são compostos por cima.
- Interior e cutscene não reutilizam uma câmera/target auxiliar.
- Um toggle desativa o recurso sem alterar a imagem original.
- Capturas A/B com entradas iguais reproduzem o ProperShaders sem ajustes
  específicos feitos apenas para uma screenshot.

