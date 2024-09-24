import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report
import joblib
import os

# Define the dataset path and genres
DATASET_PATH = '/Users/rhyseyre/Downloads/dataset'  # Update if different
genres = ['classical', 'jazz', 'rock', 'pop', 'hiphop', 'blues', 'country', 'disco', 'metal', 'reggae']

# Load features and labels
features_path = os.path.join(DATASET_PATH, 'features.npy')
labels_path = os.path.join(DATASET_PATH, 'labels.npy')

X = np.load(features_path)
y = np.load(labels_path)

print(f"Loaded {X.shape[0]} samples with {X.shape[1]} features each.")

# Split into training and testing sets
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print(f"Training samples: {X_train.shape[0]}")
print(f"Testing samples: {X_test.shape[0]}")

# Initialize and train the classifier
clf = RandomForestClassifier(n_estimators=100, random_state=42)
clf.fit(X_train, y_train)

# Evaluate the classifier
y_pred = clf.predict(X_test)
accuracy = accuracy_score(y_test, y_pred)
print(f"Genre Classification Accuracy: {accuracy * 100:.2f}%")

# Detailed classification report
print("Classification Report:")
print(classification_report(y_test, y_pred, target_names=genres))

# Confusion Matrix
print("Confusion Matrix:")
print(confusion_matrix(y_test, y_pred))

# Save the trained model
model_path = os.path.join(DATASET_PATH, 'genre_classifier.joblib')
joblib.dump(clf, model_path)
print(f"Trained model saved as '{model_path}'.")
