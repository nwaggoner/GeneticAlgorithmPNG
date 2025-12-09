#include <iostream>  // Used for cout, cerr, cin, endl
#include <vector>    // Used extensively for vectors
#include <random>    // Used for mt19937, uniform_int_distribution, etc.
#include <algorithm> // Used for sort()
#include <cmath>     // Used for sqrt(), abs() 
#include <iomanip>   // Used for setprecision
#include <ctime>     
#include <fstream>  
#include <string>    

// Simple PNG writer implementation (https://github.com/nothings/stb/blob/master/stb_image_write.h)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;


// Pixel structure to store RGB values
struct Pixel {
    unsigned char r, g, b;
    
    Pixel() : r(0), g(0), b(0) {}
    Pixel(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}
    
    // Calculate color difference between two pixels
    int difference(const Pixel& other) const {
        return abs(r - other.r) + abs(g - other.g) + abs(b - other.b);
    }
    
    // Mutation: randomly adjust a color channel
    void mutate(mt19937& gen, int mutationStrength) {
		uniform_int_distribution <> dist(0, 2);
		uniform_int_distribution <> changeDist(-mutationStrength, mutationStrength);
        
        int channel = dist(gen);
        int change = changeDist(gen);
        
        switch(channel) {
            case 0: r =  max(0,  min(255, (int)r + change)); break;
            case 1: g =  max(0,  min(255, (int)g + change)); break;
            case 2: b =  max(0,  min(255, (int)b + change)); break;
        }
    }
};

// Individual (image) in the population
struct Individual {
    vector<Pixel> pixels;
    double fitness;
    
    Individual() : fitness(0.0) {
        pixels.resize(32 * 32);
    }
    
    // Initialize with random pixels
    void randomize(mt19937& gen) {
        uniform_int_distribution <> colorDist(0, 255);
        
        for (auto& pixel : pixels) {
            pixel.r = colorDist(gen);
            pixel.g = colorDist(gen);
            pixel.b = colorDist(gen);
        }
    }
    
    // Calculate fitness based on similarity to target
    void calculateFitness(const vector<Pixel>& target) {
        double totalDiff = 0.0;
        double maxPossibleDiff = 32 * 32 * 255 * 3; // Maximum possible difference
        
        for (size_t i = 0; i < pixels.size(); i++) {
            totalDiff += pixels[i].difference(target[i]);
        }
        
        // Fitness is 1 - (normalized difference)
        fitness = 1.0 - (totalDiff / maxPossibleDiff);
    }
    
    // Crossover: mix two parents to create a child
    static Individual crossover(const Individual& parent1, const Individual& parent2, mt19937& gen) {
        Individual child;
        uniform_int_distribution <> methodDist(0, 2);
        
        int method = methodDist(gen);
        
        switch(method) {
            case 0: // Uniform crossover
                for (size_t i = 0; i < child.pixels.size(); i++) {
                    uniform_int_distribution<> choiceDist(0, 1);
                    if (choiceDist(gen) == 0) {
                        child.pixels[i] = parent1.pixels[i];
                    } else {
                        child.pixels[i] = parent2.pixels[i];
                    }
                }
                break;
                
            case 1: // Single point crossover
                {
                    uniform_int_distribution<> splitDist(0, child.pixels.size() - 1);
                    size_t splitPoint = splitDist(gen);
                    
                    for (size_t i = 0; i < splitPoint; i++) {
                        child.pixels[i] = parent1.pixels[i];
                    }
                    for (size_t i = splitPoint; i < child.pixels.size(); i++) {
                        child.pixels[i] = parent2.pixels[i];
                    }
                }
                break;
                
            case 2: // Average color crossover
                for (size_t i = 0; i < child.pixels.size(); i++) {
                    child.pixels[i].r = (parent1.pixels[i].r + parent2.pixels[i].r) / 2;
                    child.pixels[i].g = (parent1.pixels[i].g + parent2.pixels[i].g) / 2;
                    child.pixels[i].b = (parent1.pixels[i].b + parent2.pixels[i].b) / 2;
                }
                break;
        }
        
        return child;
    }
    
    // Mutate the individual
    void mutate(mt19937& gen, double mutationRate, int mutationStrength) {
        uniform_real_distribution<> probDist(0.0, 1.0);
        
        for (auto& pixel : pixels) {
            if (probDist(gen) < mutationRate) {
                pixel.mutate(gen, mutationStrength);
            }
        }
    }
    
    // Convert pixels to RGB array for PNG
    vector<unsigned char> toRGBArray() const {
        vector<unsigned char> rgb(32 * 32 * 3);
        
        for (size_t i = 0; i < pixels.size(); i++) {
            rgb[i * 3] = pixels[i].r;
            rgb[i * 3 + 1] = pixels[i].g;
            rgb[i * 3 + 2] = pixels[i].b;
        }
        return rgb;
    }
};

// Genetic Algorithm class
class GeneticAlgorithm {
private:
    vector <Individual> population;
    vector <Pixel> targetImage;
    mt19937 gen;
    int populationSize;
    double mutationRate;
    int mutationStrength;
    double crossoverRate;
    int generation;
    string targetName;
    
public:
    GeneticAlgorithm(int popSize, double mutRate, int mutStrength, double crossRate) : populationSize(popSize), mutationRate(mutRate), mutationStrength(mutStrength), crossoverRate(crossRate), generation(0) 
	{
        gen.seed( time(nullptr));
        targetImage.resize(32 * 32);
        initializePopulation();
    }
    
    void initializePopulation() {
        population.clear();
        population.resize(populationSize);
        
        for (auto& individual : population) {
            individual.randomize(gen);
        }
    }
    
    // Create different sample target images
    enum TargetType {GRADIENT, CIRCLE, CHECKERBOARD, STRIPES};
    
    void createSampleTarget(TargetType type = GRADIENT) {
        targetName = "";
        
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                int index = y * 32 + x;
                
                switch(type) {
                    case GRADIENT:
                        targetName = "gradient";
                        // Create a gradient pattern
                        targetImage[index] = Pixel(
                            (x * 255 / 31),          // Red increases horizontally
                            (y * 255 / 31),          // Green increases vertically
                            ((x + y) * 255 / 62)     // Blue increases diagonally
                        );
                        break;
                        
                    case CIRCLE:
                        targetName = "circle";
						{
							float centerX = 16.0f;
							float centerY = 16.0f;
							float radius = 10.0f;
							float dist = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
							
							if (dist <= radius) {
								targetImage[index] = Pixel(255, 100, 100);  // Pink circle
							} else {
								targetImage[index] = Pixel(50, 50, 150);    // Blue background
							}
						}
                        break;
                        
                    case CHECKERBOARD:
                        targetName = "checkerboard";
                        if (((x / 4) + (y / 4)) % 2 == 0) {
                            targetImage[index] = Pixel(255, 255, 255);  // White
                        } else {
                            targetImage[index] = Pixel(0, 0, 0);        // Black
                        }
                        break;
                        
                    case STRIPES:
                        targetName = "stripes";
                        if (x % 8 < 4) {
                            targetImage[index] = Pixel(255, 0, 0);      // Red stripe
                        } else {
                            targetImage[index] = Pixel(255, 255, 0);    // Yellow stripe
                        }
                        break;
                  
                }
            }
        }
    }
    
    void evaluateFitness() {
        for (auto& individual : population) {
            individual.calculateFitness(targetImage);
        }
        
        // Sort by fitness (descending)
		sort(population.begin(), population.end(), 
			 [](const Individual& a, const Individual& b) {
				 return a.fitness > b.fitness;
			 });
    }
    
    Individual selectParent() {
        // Tournament selection
        uniform_int_distribution<> tournamentDist(0, populationSize - 1);
        const int tournamentSize = 3;
        
        Individual best = population[tournamentDist(gen)];
        
        for (int i = 1; i < tournamentSize; i++) {
            Individual contender = population[tournamentDist(gen)];
            if (contender.fitness > best.fitness) {
                best = contender;
            }
        }
        
        return best;
    }
    
    void createNewGeneration() {
        vector<Individual> newPopulation;
        
        // Keep the best individual (elitism)
        newPopulation.push_back(population[0]);
        
        // Create the rest of the new population
        while (newPopulation.size() < populationSize) {
            uniform_real_distribution<> crossDist(0.0, 1.0);
            
            if (crossDist(gen) < crossoverRate) {
                Individual parent1 = selectParent();
                Individual parent2 = selectParent();
                Individual child = Individual::crossover(parent1, parent2, gen);
                
                child.mutate(gen, mutationRate, mutationStrength);
                newPopulation.push_back(child);
            } else {
                Individual parent = selectParent();
                Individual child = parent;
                child.mutate(gen, mutationRate, mutationStrength);
                newPopulation.push_back(child);
            }
        }
        
        population = newPopulation;
        generation++;
    }
    
    void run(int maxGenerations, double targetFitness) {
		cout << "Starting genetic algorithm to evolve " << targetName << "..." <<  endl;
		cout << "Population: " << populationSize <<  endl;
		cout << "Mutation rate: " << mutationRate <<  endl;
		cout << "Target fitness: " << targetFitness <<  endl;
        
        evaluateFitness();
        
        cout << "\nGeneration 0: Best fitness = " <<  fixed <<  setprecision(4) << population[0].fitness <<  endl;
        
        // Save initial random image
        saveBestImagePNG("initial_random.png");
        
        while (generation < maxGenerations && population[0].fitness < targetFitness) {
            createNewGeneration();
            evaluateFitness();
            
            if (generation % 100 == 0) {
                 cout << "Generation " << generation << ": Best fitness = " <<  fixed <<  setprecision(4) << population[0].fitness <<  endl;
                
                // Save progress image every 500 generations
                if (generation % 500 == 0) {
                    string filename = "progress_gen_" +  to_string(generation) + ".png";
                    saveBestImagePNG(filename);
                }
            }
        }
        
		cout << "\n" <<  string(50, '=') <<  endl;
		cout << "EVOLUTION COMPLETE!" <<  endl;
		cout <<  string(50, '=') <<  endl;
		cout << "Generations: " << generation <<  endl;
		cout << "Best fitness achieved: " <<  fixed <<  setprecision(4) << population[0].fitness <<  endl;
		cout << "Similarity to target: " << (population[0].fitness * 100) << "%" <<  endl;
    }
    
    bool saveBestImagePNG(const string& filename) {
         vector<unsigned char> rgb = population[0].toRGBArray();
        
        // stbi_write_png returns 0 on failure
        int result = stbi_write_png(filename.c_str(), 32, 32, 3, rgb.data(), 32 * 3);
        
        if (result) {
            cout << "Saved: " << filename <<  endl;
            return true;
        } else {
            cerr << "Failed to save PNG: " << filename <<  endl;
            return false;
        }
    }
    
    bool saveTargetImagePNG(const string& filename) {
        vector<unsigned char> rgb(32 * 32 * 3);
        for (size_t i = 0; i < targetImage.size(); i++) {
            rgb[i * 3] = targetImage[i].r;
            rgb[i * 3 + 1] = targetImage[i].g;
            rgb[i * 3 + 2] = targetImage[i].b;
        }
        
        int result = stbi_write_png(filename.c_str(), 32, 32, 3, rgb.data(), 32 * 3);
        
        if (result) {
            cout << "Saved: " << filename <<  endl;
            return true;
        } else {
            cerr << "Failed to save PNG: " << filename <<  endl;
            return false;
        }
    }
    
    const  vector<Pixel>& getBestPixels() const { return population[0].pixels; }
    const  vector<Pixel>& getTargetPixels() const { return targetImage; }
    const  string& getTargetName() const { return targetName; }
    int getGeneration() const { return generation; }
};


int main() {
    // Display banner
	cout <<  string(60, '=') <<  endl;
	cout << "          IMAGE EVOLUTION WITH GENETIC ALGORITHM" <<  endl;
	cout <<  string(60, '=') <<  endl;
    
    // Genetic Algorithm parameters
    const int POPULATION_SIZE = 200;
    const double MUTATION_RATE = 0.03;
    const int MUTATION_STRENGTH = 25;   // Effects rgb value mutation
    const double CROSSOVER_RATE = 0.6;  // Probability that a crossover will take place versus a mutation.
    const int MAX_GENERATIONS = 5000; 
    const double TARGET_FITNESS = 0.96; // Results reach a limit at 0.96 in some testing. 
    

    GeneticAlgorithm ga(POPULATION_SIZE, MUTATION_RATE, MUTATION_STRENGTH, CROSSOVER_RATE);
    
    // Choose target pattern
	cout << "\nChoose target pattern to evolve:" <<  endl;
	cout << "1. Color Gradient" <<  endl;
	cout << "2. Circle" <<  endl;
	cout << "3. Checkerboard" <<  endl;
	cout << "4. Stripes" <<  endl;
	cout << "Enter choice (1-4): ";
    
    int choice;
    cin >> choice;
    
    GeneticAlgorithm::TargetType targetType;
    
    switch(choice) {
        case 1: targetType = GeneticAlgorithm::GRADIENT; break;
        case 2: targetType = GeneticAlgorithm::CIRCLE; break;
        case 3: targetType = GeneticAlgorithm::CHECKERBOARD; break;
        case 4: targetType = GeneticAlgorithm::STRIPES; break;
        default: targetType = GeneticAlgorithm::GRADIENT; break;
    }
    
    // Create target image
    cout << "\nCreating target image..." <<  endl;
    ga.createSampleTarget(targetType);
    
	// Save target image
    ga.saveTargetImagePNG("target_image.png");
    
   
    // Run the genetic algorithm
    cout << "\n" <<  string(60, '=') <<  endl;
    cout << "STARTING EVOLUTION PROCESS" <<  endl;
    cout <<  string(60, '=') <<  endl;
    
    ga.run(MAX_GENERATIONS, TARGET_FITNESS);
    
    // Save the final best image
    cout << "\n" <<  string(60, '=') <<  endl;
    cout << "SAVING RESULTS" <<  endl;
    cout <<  string(60, '=') <<  endl;
    
    ga.saveBestImagePNG("best_final.png");
    
    

	cout << "\n" <<  string(60, '=') <<  endl;
	cout << "FILES CREATED:" <<  endl;
	cout <<  string(60, '=') <<  endl;
	cout << "1. target_image.png    - The target image to evolve toward" <<  endl;
	cout << "2. initial_random.png  - Random starting image (generation 0)" <<  endl;
	cout << "3. best_final.png      - Best evolved image after " << ga.getGeneration() << " generations" <<  endl;
	cout << "4. progress_gen_*.png  - Progress images every 500 generations" <<  endl;

    // Create a simple HTML viewer
    ofstream html("view_images.html");
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "    <title>Genetic Algorithm Image Evolution Results</title>\n";
    html << "    <style>\n";
    html << "        body { font-family: Arial, sans-serif; margin: 40px; }\n";
    html << "        .container { display: flex; flex-wrap: wrap; gap: 20px; }\n";
    html << "        .image-box { text-align: center; border: 1px solid #ccc; padding: 10px; }\n";
    html << "        img { border: 1px solid #000; image-rendering: pixelated; width: 320px; height: 320px; }\n";
    html << "        h1 { color: #333; }\n";
    html << "        .info { background: #f0f0f0; padding: 15px; margin: 20px 0; }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "    <h1>Genetic Algorithm Image Evolution Results</h1>\n";
    html << "    <div class='info'>\n";
    html << "        <p>Target Pattern: " << ga.getTargetName() << "</p>\n";
    html << "        <p>Generations: " << ga.getGeneration() << "</p>\n";
    html << "        <p>Images are 32x32 pixels, scaled 10x for viewing</p>\n";
    html << "    </div>\n";
    html << "    <div class='container'>\n";
    html << "        <div class='image-box'>\n";
    html << "            <h3>Target Image</h3>\n";
    html << "            <img src='target_image.png' alt='Target'>\n";
    html << "        </div>\n";
    html << "        <div class='image-box'>\n";
    html << "            <h3>Initial Random</h3>\n";
    html << "            <img src='initial_random.png' alt='Initial'>\n";
    html << "        </div>\n";
    html << "        <div class='image-box'>\n";
    html << "            <h3>Final Evolved</h3>\n";
    html << "            <img src='best_final.png' alt='Final'>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    <p>Open this HTML file in any web browser to view the images.</p>\n";
    html << "</body>\n";
    html << "</html>\n";
    html.close();
   
    
	cout << "\n" <<  string(60, '=') <<  endl;
	cout << "EVOLUTION COMPLETE! Check the generated PNG/HTML files." <<  endl;
	cout <<  string(60, '=') <<  endl;
    
    return 0;
}
